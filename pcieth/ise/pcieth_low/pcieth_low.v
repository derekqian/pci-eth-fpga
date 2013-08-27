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
module ledsw(W, R, DATA,         // from higher FPGA                                                                                                          
             LED, SW            // leds and switches                                                                                                         
           );

   input       W;
   input       R;
   inout [7:0] DATA;
   output [7:0] LED;
   input [7:0]  SW;

   reg [7:0]    LED;
	reg [7:0]    DATA_reg;
   always @(posedge W) begin
      if(~R)                    // this is write command                                                                                                     
        LED <= DATA;
   end

//   always @(posedge R) begin
//      if(~W)                    // this is read command                                                                                                      
//        DATA_reg <= SW;
//   end

//assign LED = DATA;
//assign DATA = DATA_reg;
endmodule
