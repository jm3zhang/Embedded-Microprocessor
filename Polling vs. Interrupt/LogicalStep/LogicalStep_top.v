// Copyright (C) 2016 Trevor Smouter


`define LCD
`define LED
`define SlideSwitches
`define PushButtons
`define SevenSegment
`define AudioCodec
`define UART
`define SDCard 
`define SDRAM 

module LogicalStep_top
(
	input				rst_n,		//reset in
	input          clkin_50,	//clock in

`ifdef LCD
	inout		[7:0] lcd_d,
	output			lcd_en,
	output			lcd_rw,
	output			lcd_rs,
`endif

`ifdef LED
	output	[7:0]	leds,
`endif

`ifdef SlideSwitches
	input		[7:0]	sw,
`endif

`ifdef PushButtons
	input		[3:0]	pb,
`endif

`ifdef SevenSegment
	output	[7:0]	seg7_data,
	output			seg7_char1,
	output			seg7_char2,
`endif

`ifdef AudioCodec
	output			aud_scl,
	inout				aud_sda,
	output			aud_mclk,
	input				aud_bclk,
	output			aud_dac_dat,
	input				aud_dac_lrck,
	input				aud_adc_dat,
	input				aud_adc_lrck,	
`endif

`ifdef UART
	input				uart_rx,
	output			uart_tx,
`endif

`ifdef SDCard
	inout				sd_cmd,
	output			sd_clk,
	inout				sd_dat3,
	inout				sd_dat0,
`endif

`ifdef SDRAM
	output	[12:0]sdram_a,
	output	[1:0]	sdram_ba,
	output			sdram_clk,
	output			sdram_cke,
	output			sdram_cs_n,
	output			sdram_ras_n,
	output			sdram_cas_n,
	inout		[15:0]sdram_dq,
	output	[1:0] sdram_dqm,
	output			sdram_we_n
`endif


);

	//these are used as intermediate signals
	wire 				stimulus;
	wire				response;

	

//place Qsys instance below here


		
//place Qsys instance above here
		
	

endmodule
