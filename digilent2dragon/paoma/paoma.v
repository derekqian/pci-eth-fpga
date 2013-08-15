`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:24:03 07/31/2013 
// Design Name: 
// Module Name:    paoma 
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
module paoma(clk, 
    outdata,led);

   input  clk;       // clk = 100,000,000Hz
   output reg [9:0] outdata;
	output reg [3:0] led;

   reg clk1s;        // ~ 1s
   reg [25:0] cnt;   // counter for clk1s

   always @(posedge clk)
     begin
        if(cnt == 0)
          clk1s <= ~clk1s;
        cnt <= cnt + 1;
     end

   always @(posedge clk1s)
     begin
        if(outdata == 0 || outdata == 10'b10_0000_0000)
          outdata <= 1;
		  else
          outdata <= outdata << 1;
     end
	  
always @(outdata)
  case (outdata)
    10'b00_0000_0001 : led <= 4'b0001;
    10'b00_0000_0010 : led <= 4'b0010;
    10'b00_0000_0100 : led <= 4'b0011;
    10'b00_0000_1000 : led <= 4'b0100;
    10'b00_0001_0000 : led <= 4'b0101;
    10'b00_0010_0000 : led <= 4'b0110;
    10'b00_0100_0000 : led <= 4'b0111;
    10'b00_1000_0000 : led <= 4'b1000;
    10'b01_0000_0000 : led <= 4'b1001;
    10'b10_0000_0000 : led <= 4'b1010;
    default:           led <= 4'b1111;
  endcase

endmodule
