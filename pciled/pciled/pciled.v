// Very simple PCI target
// Just 3 flipflops for the PCI logic, plus one to hold the state of an LED

module PCI(CLK, RSTn, FRAMEn, AD, CBE, IRDYn, TRDYn, DEVSELn, LED);

   input CLK, RSTn, FRAMEn, IRDYn;
   input [31:0] AD;
   input [3:0] 	CBE;
   inout 	TRDYn, DEVSELn;
   output 	LED;

   parameter IO_address = 32'h00000200;
      // we respond to an "IO write" at this address
   parameter CBECD_IOWrite = 4'b0011;

   ////////////////////////////////////////////////////
   reg 		Transaction;
   wire 	TransactionStart = ~Transaction & ~FRAMEn;
   wire 	TransactionEnd = Transaction & FRAMEn & IRDYn;
   wire 	Targeted = TransactionStart & (AD==IO_address) & (CBE==CBECD_IOWrite);
   wire 	LastDataTransfer = FRAMEn & ~IRDYn & ~TRDYn;

   always @(posedge CLK or negedge RSTn)
     if(~RSTn) Transaction <= 0;
     else
       case(Transaction)
	 1'b0: Transaction <= TransactionStart;
	 1'b1: Transaction <= ~TransactionEnd;
       endcase // case (Transaction)

   reg 		DevSelOE;

   always @(posedge CLK or negedge RSTn)
     if(~RSTn) DevSelOE <= 0;
     else
       case(Transaction)
	 1'b0: DevSelOE <= Targeted;
	 1'b1: if(TransactionEnd) DevSelOE <= 1'b0;
       endcase // case (Transaction)

   reg 		DevSel;

   always @(posedge CLK or negedge RSTn)
     if(~RSTn) DevSel <= 0;
     else
       case(Transaction)
	 1'b0: DevSel <= Targeted;
	 1'b1: DevSel <= DevSel & ~LastDataTransfer;
       endcase // case (Transaction)

   assign DEVSELn = DevSelOE ? ~DevSel : 1'bZ;
   assign TRDYn = DevSelOE ? ~DevSel : 1'bZ;

   wire 	DataTransfer = DevSel & ~IRDYn & ~TRDYn;

   reg 		LED;
   always @(posedge CLK) if(DataTransfer) LED <= AD[0];

   endmodule