`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    16:02:36 08/26/2013 
// Design Name: 
// Module Name:    pcieth_low 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module ledsw(reset, W, R, DATA, // from higher FPGA                                                
	     LED, SW     // leds and switches                                                    
	     );

   input    reset;
   input    W;
   input    R;
   inout [7:0] DATA;
   output [7:0] LED;
   input [7:0] 	SW;

   reg [7:0] 	LED;
   reg [7:0] 	DATA_reg;
   reg 		DATA_reg_en;
   reg 		state;
   
   always @(negedge reset or posedge W or posedge R)  begin 
      if(~reset) begin
	 state <= 1'b0;
	 DATA_reg_en <= 0;
      end
      else if(W) begin 
	 state <= ~ state;
	 if(~state)
      	   DATA_reg_en <= 0;
	 else begin
	    if(~R)                    // this is write command                                       
	      LED <= DATA;
	 end
      end
      else begin
	 
	 if(~W) begin                   // this is read command                                               
            DATA_reg_en <= 1;
	    //LED <= SW;
	 end
      end
   end

   assign DATA = DATA_reg_en ? SW : 8'hZZ;
   //assign LED = {W, R, DATA[5:0]};
endmodule
