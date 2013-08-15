`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    15:38:36 07/18/2013 
// Design Name: 
// Module Name:    testhw1 
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
module ten24 (indata, outdata);

   input  [9:0] indata;
   output [3:0] outdata;

reg [3:0] outdata;

always @(indata)
  case (indata)
    10'b00_0000_0001 : outdata <= 4'b0001;
    10'b00_0000_0010 : outdata <= 4'b0010;
    10'b00_0000_0100 : outdata <= 4'b0011;
    10'b00_0000_1000 : outdata <= 4'b0100;
    10'b00_0001_0000 : outdata <= 4'b0101;
    10'b00_0010_0000 : outdata <= 4'b0110;
    10'b00_0100_0000 : outdata <= 4'b0111;
    10'b00_1000_0000 : outdata <= 4'b1000;
    10'b01_0000_0000 : outdata <= 4'b1001;
    10'b10_0000_0000 : outdata <= 4'b1010;
    default:           outdata <= 4'b1111;
  endcase
endmodule
