// PCI example design for the Dragon board - fpga4fun.com
// Plug-and-play - use it with the WDM DragonPCI driver
//
// Things that should be done for PCI compliance:
// 1. Generate the PAR signal on reads
// 2. Look at the CBE signals during transactions (byte-enables during transactions)
//    Right now we don't look at the CBEs and we assume that they are always enabled (i.e. 32-bits transactions).

module pcieth
(
  // PCI pins
  PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_AD, PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_DEVSELn, PCI_IDSEL, PCI_STOPn, PCI_INTAn, PCI_REQn, PCI_GNTn,

  // other pins
  LED,

  // unused PCI pins
  PCI_PAR, PCI_LOCKn, PCI_PERRn, PCI_SERRn,

  // to lower board.
  LOW_w, LOW_r, LOW_data,

  // USB
  CLK24, /*USB_FWRn, */USB_FRDn, USB_D
);
// Choose which space you want to use. 
// Define one (or both) of the following
`define PCI_IOSPACE   // implemented using BAR0
`define PCI_MEMSPACE  // implemented using BAR1
`define INTERRUPT
`define FASTBACKTOBACK
`define AUTOINCADDR

// Specify the Vendor/Device ID
// Make sure these values match the ones used in the INF file
parameter VENDOR_ID = 16'h0100;
parameter DEVICE_ID = 16'h0001;

////////////////////////////////////////////////////////////////////////////////
parameter PCI_CBECD_IORead                = 4'b0010;	// 2
parameter PCI_CBECD_IOWrite               = 4'b0011;	// 3
parameter PCI_CBECD_MEMRead               = 4'b0110;	// 6
parameter PCI_CBECD_MEMWrite              = 4'b0111;	// 7
parameter PCI_CBECD_CSRead                = 4'b1010;	// A
parameter PCI_CBECD_CSWrite               = 4'b1011;	// B
parameter PCI_CBECD_MEMReadMultiple       = 4'b1100;	// C
parameter PCI_CBECD_MEMReadLine           = 4'b1110;	// E
parameter PCI_CBECD_MEMWriteAndInvalidate = 4'b1111;	// F

input PCI_CLK, PCI_RSTn, PCI_IDSEL, PCI_GNTn;
inout [31:0] PCI_AD;
inout [3:0] PCI_CBE;
inout PCI_FRAMEn, PCI_IRDYn, PCI_TRDYn, PCI_DEVSELn, PCI_STOPn, PCI_INTAn, PCI_REQn;
output [1:0] LED;

// unused pins still need to be present because they are assigned the "PCI33_5" IO standard in the UCF file
input PCI_PAR, PCI_LOCKn, PCI_PERRn, PCI_SERRn;  // unused pins

output LOW_w, LOW_r;
inout [7:0] LOW_data;

input CLK24, USB_FRDn;//, USB_FWRn;
inout [7:0] USB_D;

////////////////////////////////////////////////////////////////////////////////
wire PCI_DataTransfer = ~PCI_IRDYn & ~PCI_TRDYn;
wire PCI_LastDataTransfer = PCI_DataTransfer & PCI_FRAMEn;

reg PCI_Transaction, PCI_WasLastDataTransfer;
always @(posedge PCI_CLK) PCI_WasLastDataTransfer <= PCI_LastDataTransfer;

wire PCI_TransactionStart_Initial = ~PCI_Transaction & ~PCI_FRAMEn;
`ifdef FASTBACKTOBACK
wire PCI_TransactionStart_BackToBack = PCI_WasLastDataTransfer & ~PCI_FRAMEn;
`else
wire PCI_TransactionStart_BackToBack = 1'b0;
`endif

wire PCI_Idle = PCI_FRAMEn & PCI_IRDYn;
wire PCI_TransactionStart = PCI_TransactionStart_Initial | PCI_TransactionStart_BackToBack;
wire PCI_TransactionEnd = PCI_Transaction & PCI_Idle;

always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Transaction <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_Transaction <= PCI_TransactionStart;
  1'b1: PCI_Transaction <= ~PCI_TransactionEnd;
endcase

reg PCI_CSCMD_IOenabled;  reg [ 9:0] PCI_CSBAR0;
reg PCI_CSCMD_MEMenabled; reg [15:0] PCI_MEMBAR1;
wire PCI_CBE_IO  = (PCI_CBE==PCI_CBECD_IORead) | (PCI_CBE==PCI_CBECD_IOWrite);
wire PCI_CBE_MEM = (PCI_CBE==PCI_CBECD_MEMRead) | (PCI_CBE==PCI_CBECD_MEMReadMultiple) | (PCI_CBE==PCI_CBECD_MEMReadLine) | 
                   (PCI_CBE==PCI_CBECD_MEMWrite) | (PCI_CBE==PCI_CBECD_MEMWriteAndInvalidate);
wire PCI_CBE_CS  = (PCI_CBE==PCI_CBECD_CSRead) | (PCI_CBE==PCI_CBECD_CSWrite);

wire PCI_Targeted = PCI_TransactionStart & (
    (PCI_CBE_IO  & PCI_CSCMD_IOenabled  & (PCI_AD[15: 6]==PCI_CSBAR0 ) & (PCI_AD[1:0]==0)) | 
    (PCI_CBE_MEM & PCI_CSCMD_MEMenabled & (PCI_AD[31:16]==PCI_MEMBAR1)                   ) | 
    (PCI_CBE_CS  & PCI_IDSEL                                           & (PCI_AD[1:0]==0))
	);

////////////////////////////////////////////////////////////////////////////////
// When a transaction starts, the address is available for us to register
//reg [4:0] PCI_TransactionAddr;
reg [3:0] PCI_TransactionAddr;
always @(posedge PCI_CLK)
//if(PCI_TransactionStart) PCI_TransactionAddr <= PCI_AD[6:2];
if(PCI_TransactionStart) PCI_TransactionAddr <= PCI_AD[5:2];

`ifdef AUTOINCADDR
else if(PCI_DataTransfer) PCI_TransactionAddr <= PCI_TransactionAddr + 5'h1;
`endif

reg PCI_Transaction_Read_nWrite, PCI_IOSpace, PCI_MEMSpace, PCI_ConfSpace;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) {PCI_Transaction_Read_nWrite, PCI_IOSpace, PCI_MEMSpace, PCI_ConfSpace} <= 0; else
if(PCI_TransactionStart)
begin
  PCI_Transaction_Read_nWrite <= ~PCI_CBE[0];
  PCI_IOSpace   <= PCI_CBE_IO;
  PCI_MEMSpace  <= PCI_CBE_MEM;
  PCI_ConfSpace <= PCI_CBE_CS;
end

// Should we claim the transaction?
reg PCI_DevSelOE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSelOE <= 1'b0; else
if(~PCI_Transaction) PCI_DevSelOE <= PCI_Targeted; else
if(PCI_TransactionEnd) PCI_DevSelOE <= 1'b0;

// PCI_DEVSELn should be asserted up to the last data transfer
reg PCI_DevSel;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSel <= 1'b0; else
if(PCI_TransactionStart) PCI_DevSel <= PCI_Targeted;
else PCI_DevSel <= PCI_DevSel & ~PCI_LastDataTransfer;

// PCI_TRDYn is asserted during the whole PCI_Transaction because we don't need wait-states
// For read transaction, delay by one clock to allow for the turnaround-cycle
reg PCI_TargetReady;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_TargetReady <= 1'b0; else
if(PCI_TransactionStart) PCI_TargetReady <= PCI_Targeted & PCI_CBE[0];  // active now on write
else PCI_TargetReady <= PCI_DevSel & ~PCI_LastDataTransfer;  // next cycle on reads

wire PCI_Stop = 1'b0;  // allow bursts
// Otherwise Stop is used to "disconnect with data" (avoid burst)
/*
reg PCI_Stop;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Stop <= 1'b0; else
if(PCI_TransactionStart) PCI_Stop <= PCI_Targeted & PCI_CBE[0];  // active now on write, next cycle on reads
else PCI_Stop <= PCI_DevSel & ~PCI_FRAMEn;
*/

////////////////////////////////////////////////////////////////////////////////
// Drive the AD bus on reads only, and allow for the turnaround cycle
reg PCI_AD_OE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_AD_OE <= 1'b0;
else PCI_AD_OE <= PCI_DevSel & PCI_Transaction_Read_nWrite & ~PCI_LastDataTransfer;

`ifdef INTERRUPT
reg PCI_INTA;
wire PCI_REQ = 1'b0;
`else
wire PCI_INTA = 1'b0;
wire PCI_REQ = 1'b0;
`endif

// Claim the PCI_Transaction
assign PCI_DEVSELn = PCI_DevSelOE ? ~PCI_DevSel : 1'bZ;
assign PCI_TRDYn = PCI_DevSelOE ? ~PCI_TargetReady : 1'bZ;
assign PCI_STOPn = PCI_DevSelOE ? ~PCI_Stop : 1'bZ;
assign PCI_INTAn = PCI_INTA ? 1'b0 : 1'bZ;
assign PCI_REQn = PCI_RSTn ? ~PCI_REQ : 1'bZ;

////////////////////////////////////////////////////////////////////////////////
// Configuration space
wire PCI_CSTransferWrite = PCI_DevSel & PCI_ConfSpace & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;
//wire PCI_CSTransferRead  = PCI_DevSel & PCI_ConfSpace &  PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;

`ifdef PCI_IOSPACE
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSCMD_IOenabled <= 1'b0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==5'h01)) PCI_CSCMD_IOenabled <= PCI_AD[0];
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSBAR0 <= 10'h000; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==5'h04)) PCI_CSBAR0 <= PCI_AD[15:6];
`endif

`ifdef PCI_MEMSPACE
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_CSCMD_MEMenabled <= 1'b0; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==5'h01)) PCI_CSCMD_MEMenabled <= PCI_AD[1];
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) PCI_MEMBAR1 <= 16'h0000; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==5'h05)) PCI_MEMBAR1 <= PCI_AD[31:16];
`endif

`ifdef INTERRUPT
reg [7:0] InterruptLine;
always @(posedge PCI_CLK or negedge PCI_RSTn) if(~PCI_RSTn) InterruptLine <= 8'h00; else if(PCI_CSTransferWrite & (PCI_TransactionAddr==5'h0F)) InterruptLine <= PCI_AD[7:0];
`endif

reg [31:0] PCI_CSread;
always @(PCI_TransactionAddr, PCI_CSCMD_IOenabled, PCI_CSBAR0, PCI_CSCMD_MEMenabled, PCI_MEMBAR1)
case(PCI_TransactionAddr)
  5'h00: PCI_CSread <=  {DEVICE_ID, VENDOR_ID};
  5'h01: begin 
    PCI_CSread[15: 0] <= {14'h000, PCI_CSCMD_MEMenabled, PCI_CSCMD_IOenabled};  // 04h: COMMAND
    PCI_CSread[31:16] <= {5'b00000, 2'b00, 9'b000000000};  // 06h: STATUS (DEVSEL=00=fast)
  end
  5'h02: PCI_CSread <= {32'h08800000};  // 08h: Device "Class Code"
  5'h03: PCI_CSread <= {32'h00000000};  // 0Ch: BIST / HeaderType / LatencyTimer / CacheLineSize
`ifdef PCI_IOSPACE
  5'h04: PCI_CSread <= {16'h0000, PCI_CSBAR0, 6'b000001};  // 10h: BAR0
`endif
`ifdef PCI_MEMSPACE
// 64 Ko
  5'h05: PCI_CSread <= {PCI_MEMBAR1, 12'h000, 4'b0000};    // 14h: BAR1
// 128 Mo
//  5'h05: PCI_CSread <= {PCI_MEMBAR1, 23'h000000, 4'b0000};    // 14h: BAR1
`endif
  5'h0B: PCI_CSread <= {32'h00000000};  // 2Ch: SubsystemID / SubsystemVendorID
`ifdef INTERRUPT
  5'h0F: PCI_CSread <= {8'h00, 8'h00, 8'h01, InterruptLine};  // 3Ch: Max_Lat / Min_Gnt / InterruptPin / InterruptLine
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


// reg [31:0] REG_id;
reg [31:0] REG_led;
reg [31:0] REG_status;
reg [31:0] REG_switch;
reg [31:0] REG_WRdata;
wire WR_en;

// write
always @(posedge PCI_CLK) 
if(PCI_IOTransferWrite) 
begin
case(PCI_TransactionAddr)
  4'h1: REG_led <= {29'h00000000, PCI_AD[5:3]};
  4'h2: REG_status <= PCI_AD;
  4'h4: REG_WRdata <= PCI_AD;
  default:;
endcase
end
else if(PCI_MEMTransferWrite)
begin
case(PCI_TransactionAddr)
  4'h1: REG_led <= {29'h00000000, PCI_AD[2:0]};
  4'h2: REG_status <= PCI_AD;
  4'h4: REG_WRdata <= PCI_AD;
  default:;
endcase
end

// read
reg [31:0] PCI_DATAread;
always @(PCI_IOSpace, PCI_MEMSpace, PCI_TransactionAddr, REG_led, REG_status, REG_switch)
if(PCI_IOSpace)
begin
case(PCI_TransactionAddr)
  4'h0: PCI_DATAread <=  32'h00004F49;
  4'h1: PCI_DATAread <= {26'h0000000, REG_led[2:0], 3'b000};
  4'h2: PCI_DATAread <= REG_status;
  4'h3: PCI_DATAread <= REG_switch;
  default:;
endcase
end
else if(PCI_MEMSpace)
begin
case(PCI_TransactionAddr)
  4'h0: PCI_DATAread <=  32'h004D454D;
  4'h1: PCI_DATAread <= {29'h00000000, REG_led[2:0]};
  4'h2: PCI_DATAread <= REG_status;
  4'h3: PCI_DATAread <= REG_switch;
  default:;
endcase
end
wire [31:0] PCI_read = PCI_WorkSpace ? PCI_DATAread : PCI_CSread;
assign PCI_AD = PCI_AD_OE ? PCI_read : 32'hZZZZZZZZ;


//reg [24:0] cnt_haha;
//always @ (posedge PCI_CLK) cnt_haha <= cnt_haha + 1;
wire [7:0] RDdata_unused;   // TODO: remove this 
wire [1:0] LED_unused;
txrx U_txtx(
            .reset      (PCI_RSTn),
            .clk        (PCI_CLK),
            .RDdata     (RDdata_unused),
            .RDen       (1'b0),
            .WRdata     (REG_WRdata),
            .WRen       (PCI_TransferWrite),
				//.WRen       (1'b1),
            .W          (LOW_w),
            .R          (LOW_r),
            .DATA       (LOW_data),
				//.LED        (LED)
				.LED        (LED_unused)
            );


`ifdef INTERRUPT
reg [24:0] counter; always @(posedge PCI_CLK) counter<=counter+25'h1;
always @(posedge PCI_CLK) PCI_INTA<=&counter;
//always @(posedge PCI_CLK) PCI_INTA<=counter[23];
//always @(posedge PCI_CLK) if(PCI_TransferWrite) PCI_INTA <= (PCI_AD==32'h12345678);
`endif

////////////////////////////////////////////////////////////////////////////////
assign LED[0] = REG_led[0];
assign LED[1] = PCI_INTA;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Logic analyzer (LA) part
//
wire trigger = PCI_Targeted;	// define here when the LA triggers

reg [47:0] RAM_LA [255:0];  // acquisition RAM
reg [7:0] addra;  // we need an 8 bits address (store the PCI bus for 256 clocks)
reg acquisition; always @(posedge PCI_CLK) if(&addra) acquisition <= 1'b0; else if(trigger) acquisition <= 1'b1;

// We capture the following:
// [31:0] PCI_AD  // 32
// [3:0] PCI_CBE  //  4
// PCI_FRAMEn, PCI_IRDYn  //  2
// PCI_TRDYn, PCI_DEVSELn //  2
// PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn  // 8
// Total: 48
wire [47:0] disr = {
  PCI_AD, 
  PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_FRAMEn, PCI_DEVSELn, 
  PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn};

// Use 48 shift-registers to delay the 48 signals and get pre-trigger acquisition
// But the following doesn't work with XST:
//  reg [47:0] SRDR16 [15:0];  always @(posedge PCI_CLK) {SRDR16} <= {SRDR16[15:1], disr};
// so we use that instead to delay the signals:
wire [47:0] dosr;
SR16_8 SR0(.d(disr[ 7: 0]), .q(dosr[ 7: 0]), .clk(PCI_CLK));
SR16_8 SR1(.d(disr[15: 8]), .q(dosr[15: 8]), .clk(PCI_CLK));
SR16_8 SR2(.d(disr[23:16]), .q(dosr[23:16]), .clk(PCI_CLK));
SR16_8 SR3(.d(disr[31:24]), .q(dosr[31:24]), .clk(PCI_CLK));
SR16_8 SR4(.d(disr[39:32]), .q(dosr[39:32]), .clk(PCI_CLK));
SR16_8 SR5(.d(disr[47:40]), .q(dosr[47:40]), .clk(PCI_CLK));

// write to the RAM
always @(posedge PCI_CLK) if(acquisition) RAM_LA[addra] <= dosr;
always @(posedge PCI_CLK) if(acquisition) addra <= addra + 1;

// read the RAM from the USB
reg [10:0] addrb;  always @(posedge CLK24) if(~USB_FRDn) addrb <= addrb + 1;
reg [7:0] addr_regb;  always @(posedge CLK24) addr_regb <= addrb[10:3];
wire [47:0] dob = RAM_LA[addr_regb];

reg [7:0] USB_Data;
always @(posedge CLK24)
case(addrb[2:0])
  3'h0: USB_Data <= dob[ 7: 0];
  3'h1: USB_Data <= dob[15: 8];
  3'h2: USB_Data <= dob[23:16];
  3'h3: USB_Data <= dob[31:24];
  3'h4: USB_Data <= dob[39:32];
  3'h5: USB_Data <= dob[47:40];
  3'h6: USB_Data <= 8'h01;
  3'h7: USB_Data <= 8'h02;
endcase

assign USB_D = ~USB_FRDn ? USB_Data : 8'hZZ;
endmodule


////////////////////////////////////////////////////////////////////////////////
module SR16_8(d, q, clk);
input clk;
input [7:0] d;
output [7:0] q;

SR16 SR_0(.d(d[0]), .q(q[0]), .clk(clk));
SR16 SR_1(.d(d[1]), .q(q[1]), .clk(clk));
SR16 SR_2(.d(d[2]), .q(q[2]), .clk(clk));
SR16 SR_3(.d(d[3]), .q(q[3]), .clk(clk));
SR16 SR_4(.d(d[4]), .q(q[4]), .clk(clk));
SR16 SR_5(.d(d[5]), .q(q[5]), .clk(clk));
SR16 SR_6(.d(d[6]), .q(q[6]), .clk(clk));
SR16 SR_7(.d(d[7]), .q(q[7]), .clk(clk));
endmodule


////////////////////////////////////////////////////////////////////////////////
module SR16(d, q, clk);
input d, clk;
output q;

reg [15:0] data;
//assign q = data[15];
assign q = data[3];
always @(posedge clk) data[15:0] <= {data[14:0], d};
endmodule

///////////////////////////////////////////////////////////////////////////////
// this is the transmit and receive module from PCI
module txrx (reset, clk, RDdata, RDen, WRdata, WRen, W, R, DATA, LED);
   input        reset;
   input        clk;
   // from PCI module
   output [7:0] RDdata;         // read data
   input        RDen;           // read enable signal
   input [7:0] WRdata;         // write data
   input        WRen;           // write enable signal

   // to lower FPGA
   output       W;              // write clk
   output       R;              // read clk
   inout [7:0]  DATA;           // w/r data
	output [1:0] LED;

   reg [9:0]    cnt;
   reg W;

   always@(posedge clk or negedge reset) begin
	if (~reset) begin
	    cnt <= 10'h3ff;
		 W <= 1'b0;		 
	end
	else begin
	case(cnt)
		10'h000: begin
			W <= 1'b1;
			cnt<= cnt + 1;
		end
		10'h200: begin
		   W <= 1'b0;
         cnt<= cnt + 1;
		end
		10'h3ff: begin
		  if(WRen)
			  cnt<= cnt + 1;
		end
		default: begin
         cnt<= cnt + 1;
		end
   endcase
	end
   end
	
//	assign W = WRen;
//   assign LED = cnt;
   assign DATA   = WRen ? WRdata : DATA;
//   assign DATA[3:0]   = WRen ? WRdata[3:0] : DATA[3:0];
//	assign DATA[5:4]   = cnt[24:23];
//	assign DATA[7:6] =2'b00;
   assign RDdata = RDen ? DATA   : RDdata;

endmodule
