// PCI example design for the Dragon board - fpga4fun.com
// Plug-and-play - use it with the WDM DragonPCI driver
//
// Things that should be done for PCI compliance:
// 1. Generate the PAR signal on reads
// 2. Look at the CBE signals during transactions (byte-enables during transactions)
//    Right now we don't look at the CBEs and we assume that they are always enabled (i.e. 32-bits transactions).

module PCI_PnP
(
  // PCI pins
  PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_AD, PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_DEVSELn, PCI_IDSEL, PCI_STOPn,

  // other pins
  LED, LED2,

  // unused PCI pins
  PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn
);

// Choose which space you want to use. 
// Define one (or both) of the following
`define PCI_IOSPACE   // implemented using BAR0
`define PCI_MEMSPACE  // implemented using BAR1

// Specify the Vendor/Device ID
// Make sure these values match the ones used in the INF file
parameter VENDOR_ID = 16'h0100;
parameter DEVICE_ID = 16'h0000;

////////////////////////////////////////////////////////////////////////////////
parameter PCI_CBECD_IORead  = 4'b0010;
parameter PCI_CBECD_IOWrite = 4'b0011;
parameter PCI_CBECD_MEMRead  = 4'b0110;
parameter PCI_CBECD_MEMWrite = 4'b0111;
parameter PCI_CBECD_CSRead  = 4'b1010;
parameter PCI_CBECD_CSWrite = 4'b1011;

input PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_IRDYn, PCI_IDSEL;
inout [31:0] PCI_AD;
input [3:0] PCI_CBE;
output PCI_TRDYn, PCI_DEVSELn, PCI_STOPn;
output LED, LED2;

// unused pins still need to be present because they are assigned the "PCI33_5" IO standard in the UCF file
input PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn;  // unused pins
// dummy equation so that unused pins are not synthesized away
wire Dummy1 = PCI_PAR | PCI_GNTn | PCI_LOCKn | PCI_PERRn | PCI_REQn | PCI_SERRn;  // always equals 1

////////////////////////////////////////////////////////////////////////////////
reg PCI_Transaction;

wire PCI_TransactionStart = ~PCI_Transaction & ~PCI_FRAMEn;
wire PCI_TransactionEnd = PCI_Transaction & PCI_FRAMEn & PCI_IRDYn;

always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Transaction <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_Transaction <= PCI_TransactionStart;
  1'b1: PCI_Transaction <= ~PCI_TransactionEnd;
endcase

reg PCI_CSCMD_IOenabled;  reg [ 9:0] PCI_CSBAR0;
reg PCI_CSCMD_MEMenabled; reg [15:0] PCI_MEMBAR1;
wire PCI_Targeted = PCI_TransactionStart & (PCI_AD[1:0]==0) & (
		( (PCI_CSCMD_IOenabled & (PCI_AD[15:6]==PCI_CSBAR0) & ((PCI_CBE==PCI_CBECD_IORead) | (PCI_CBE==PCI_CBECD_IOWrite)) ) ) | 
		( (PCI_CSCMD_MEMenabled & (PCI_AD[31:16]==PCI_MEMBAR1) & ((PCI_CBE==PCI_CBECD_MEMRead) | (PCI_CBE==PCI_CBECD_MEMWrite)) ) ) | 
		(  PCI_IDSEL & ((PCI_CBE==PCI_CBECD_CSRead) | (PCI_CBE==PCI_CBECD_CSWrite)) )
	);

// When a transaction starts, the address is available for us to register
// We just need a 4 bits address here
reg [3:0] PCI_TransactionAddr;
always @(posedge PCI_CLK) if(PCI_TransactionStart) PCI_TransactionAddr <= PCI_AD[5:2];

wire PCI_LastDataTransfer = PCI_FRAMEn & ~PCI_IRDYn & ~PCI_TRDYn;

reg PCI_Transaction_Read_nWrite, PCI_IOSpace, PCI_MEMSpace, PCI_ConfSpace;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) {PCI_Transaction_Read_nWrite, PCI_IOSpace, PCI_MEMSpace, PCI_ConfSpace} <= 0;
else
if(~PCI_Transaction)
begin
  PCI_Transaction_Read_nWrite <= ~PCI_CBE[0];
  PCI_IOSpace   <= (PCI_CBE[3:2]==2'b00);
  PCI_MEMSpace  <= (PCI_CBE[3:2]==2'b01);
  PCI_ConfSpace <= (PCI_CBE[3:2]==2'b10);
end

// Should we claim the transaction?
reg PCI_DevSelOE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSelOE <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_DevSelOE <= PCI_Targeted;
  1'b1: if(PCI_TransactionEnd) PCI_DevSelOE <= 1'b0;
endcase

// PCI_DEVSELn should be asserted up to the last data transfer
reg PCI_DevSel;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSel <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_DevSel <= PCI_Targeted;
  1'b1: PCI_DevSel <= PCI_DevSel & ~PCI_LastDataTransfer;
endcase

// PCI_TRDYn is asserted during the whole PCI_Transaction because we don't need wait-states
// For read transaction, delay by one clock to allow for the turnaround-cycle
reg PCI_TargetReady;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_TargetReady <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_TargetReady <= PCI_Targeted & PCI_CBE[0];  // active now on write, next cycle on reads
  1'b1: PCI_TargetReady <= PCI_DevSel & ~PCI_LastDataTransfer;
endcase

// Stop is used to "disconnect with data" (avoid burst)
reg PCI_Stop;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Stop <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_Stop <= PCI_Targeted & PCI_CBE[0];  // active now on write, next cycle on reads
  1'b1: PCI_Stop <= PCI_DevSel & ~PCI_FRAMEn;
endcase

// Drive the AD bus on reads only, and allow for the turnaround cycle
reg PCI_AD_OE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_AD_OE <= 0;
else
PCI_AD_OE <= PCI_DevSel & PCI_Transaction_Read_nWrite & ~PCI_LastDataTransfer;

// Claim the PCI_Transaction
assign PCI_DEVSELn = PCI_DevSelOE ? ~PCI_DevSel : 1'bZ;
assign PCI_TRDYn = PCI_DevSelOE ? ~PCI_TargetReady : 1'bZ;
assign PCI_STOPn = PCI_DevSelOE ? ~PCI_Stop : 1'bZ;

////////////////////////////////////////////////////////////////////////////////
// Configuration space
wire PCI_CSTransferWrite = PCI_DevSel & PCI_ConfSpace & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_CSTransferRead  = PCI_DevSel & PCI_ConfSpace &  PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;

`ifdef PCI_IOSPACE
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSCMD_IOenabled <= 0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==4'h1)) PCI_CSCMD_IOenabled <= PCI_AD[0];
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSBAR0 <= 0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==4'h4)) PCI_CSBAR0 <= PCI_AD[15:6];
`endif

`ifdef PCI_MEMSPACE
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSCMD_MEMenabled <= 0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==4'h1)) PCI_CSCMD_MEMenabled <= PCI_AD[1];
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_MEMBAR1 <= 0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==4'h5)) PCI_MEMBAR1 <= PCI_AD[31:16];
`endif

reg [31:0] PCI_CSread;
always @(PCI_TransactionAddr, PCI_CSCMD_IOenabled, PCI_CSBAR0, PCI_CSCMD_MEMenabled, PCI_MEMBAR1)
case(PCI_TransactionAddr)
  4'h0: PCI_CSread <=  {DEVICE_ID, VENDOR_ID};
  4'h1: PCI_CSread <= {30'h0000000, PCI_CSCMD_MEMenabled, PCI_CSCMD_IOenabled};  // STATUS / COMMAND
//  4'h2: PCI_CSread <= {32'h0880000};  // Device "Class Code"
`ifdef PCI_IOSPACE
  4'h4: PCI_CSread <= {16'h0000, PCI_CSBAR0, 6'b000001};	 // BAR0
`endif
`ifdef PCI_MEMSPACE
  4'h5: PCI_CSread <= {PCI_MEMBAR1, 12'h000, 4'b0000};  // BAR1
`endif
  default: PCI_CSread <= 32'h00000000;
endcase

////////////////////////////////////////////////////////////////////////////////
// IO or MEM space
wire PCI_WorkSpace = PCI_IOSpace | PCI_MEMSpace;

wire PCI_TransferWrite = PCI_DevSel & PCI_WorkSpace & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_TransferRead  = PCI_DevSel & PCI_WorkSpace &  PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_IOTransferWrite = PCI_DevSel & PCI_IOSpace & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_IOTransferRead  = PCI_DevSel & PCI_IOSpace &  PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_MEMTransferWrite = PCI_DevSel & PCI_MEMSpace & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_MEMTransferRead  = PCI_DevSel & PCI_MEMSpace &  PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
// Instantiate the RAM
// We use Xilinx's synthesis here (XST), which supports automatic RAM recognition
// The following code creates a distributed RAM, but a blockram could also be used (we have 2 clock cycles to get the data out)
reg [31:0] RAM [15:0];
always @(posedge PCI_CLK) if(PCI_TransferWrite) RAM[PCI_TransactionAddr] <= PCI_AD;

// now we can drive the PCI_AD bus
wire [31:0] PCI_read = PCI_WorkSpace ? RAM[PCI_TransactionAddr] : PCI_CSread;
assign PCI_AD = PCI_AD_OE ? PCI_read : 32'hZZZZZZZZ;

////////////////////////////////////////////////////////////////////////////////
reg [31:0] PCI_data; always @(posedge PCI_CLK) if(PCI_TransferWrite) PCI_data <= PCI_AD;

wire LED = PCI_data[0] & Dummy1;
wire LED2 = PCI_data[1];

endmodule
