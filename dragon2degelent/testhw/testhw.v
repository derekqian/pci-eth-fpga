`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:31:57 07/18/2013 
// Design Name: 
// Module Name:    testhw 
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
module testhw(
    input clk,
    output [9:0] dout
    );

reg [9:0] data;

reg [25:0] couter;
reg clk_1Hz;

always @(posedge clk)
begin
if(couter == 0) clk_1Hz <= ~clk_1Hz;
couter <= couter + 26'h1;
end

always @(posedge clk_1Hz)
if(data == 10'h000)
    data <= 10'h001;
//else if((data & (data - 10'h001)) != 10'h000)
//    data <= data & (data - 10'h001);
else
    data <= data<<1;

assign dout = 10'h3ff;

endmodule
