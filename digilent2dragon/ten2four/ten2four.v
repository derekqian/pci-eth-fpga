`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:49:05 07/31/2013 
// Design Name: 
// Module Name:    ten2four 
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
module ten2four(indata, outdata
    );

   input  [9:0] indata;
   output [7:0] outdata;

reg [7:0] outdata;

always @(indata)
  case (indata)
    10'b00_0000_0001 : outdata <= 8'b00000001;
    10'b00_0000_0010 : outdata <= 8'b00000010;
    10'b00_0000_0100 : outdata <= 8'b00000011;
    10'b00_0000_1000 : outdata <= 8'b00000100;
    10'b00_0001_0000 : outdata <= 8'b00000101;
    10'b00_0010_0000 : outdata <= 8'b00000110;
    10'b00_0100_0000 : outdata <= 8'b00000111;
    10'b00_1000_0000 : outdata <= 8'b00001000;
    10'b01_0000_0000 : outdata <= 8'b00001001;
    10'b10_0000_0000 : outdata <= 8'b00001010;
    default:           outdata <= 8'b00001111;
  endcase
  
endmodule
