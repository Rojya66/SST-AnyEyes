/*******************************************************************************/
module async_transmitter(
   clk,    
   datain,      
   TXD,                
   WR,
	BUSY//,
	//pl,
	//pr,
	//PLR,
	//PRR,
	//PWM_SYN,
	//UNIT
   );
	
 input   clk;   
 input   WR; 
 //input   PLR;
 //input 	PRR; 
 //input	PWM_SYN;
 //input   UNIT;
 input [15:0] datain;  
 //input [15:0] pl;
 //input [15:0] pr; 
 output   TXD,BUSY;      
 reg  [17:0] datainbuf = 18'd0;   
 reg  TI,TXD_reg;    
 reg  [7:0] bincnt = 8'd0;    
 reg  [15:0] cnt = 16'd0; 
 wire   clk_equ;   
 parameter BANDRATE = 16'd32; 
 
//parameter	CMD_SYNC = 16'h0111;
//parameter	CMD_OPEN = 16'h0555;
//parameter 	CMD_CLOZ = 16'h0AAA;	
 
always@(posedge clk)          
begin
     if(clk_equ)
      cnt <= 16'd0;
     else 
      cnt<=cnt+16'd1;
end
assign clk_equ = (cnt == (BANDRATE-16'd1));

//wire PL_start;
//reg PLR_R1,PLR_R2,PLR_R3;
//wire PR_start;
//reg PRR_R1,PRR_R2,PRR_R3;
//wire SYNC_start;
//reg SYNC_R1,SYNC_R2,SYNC_R3;

//wire UNIT_open, UNIT_close;
//reg UNIT_R1, UNIT_R2, UNIT_R3;

/*
always @ (posedge clk_equ)
begin
	if(WR_ctr == 0)
		begin
		  //UNIT_R1 <= UNIT;
		  //UNIT_R2 <= UNIT_R1;
		  //UNIT_R3 <= UNIT_R2;
		  
		  	WR_R1 <= WR;
			WR_R2 <= WR_R1;
			WR_R3 <= WR_R2;
			
			//PLR_R1 <= PLR;
			//PLR_R2 <= PLR_R1;
			//PLR_R3 <= PLR_R2;
			
			//PRR_R1 <= PRR;
			//PRR_R2 <= PRR_R1;
			//PRR_R3 <= PRR_R2;
			
			//SYNC_R1 <= PWM_SYN;
			//SYNC_R2 <= SYNC_R1;
			//SYNC_R3 <= SYNC_R2;
		end
end*/

//assign UNIT_close = (~UNIT_R3)&(UNIT_R2)&(UNIT_R1); 
//assign UNIT_open = (UNIT_R3)&(~UNIT_R2)&(~UNIT_R1); 

//assign PL_start = (PLR_R3)&(~PLR_R2)&(~PLR_R1);
//assign PR_start = (PRR_R3)&(~PRR_R2)&(~PRR_R1);
//assign SYNC_start = (SYNC_R3)&(~SYNC_R2)&(~SYNC_R1);

/*always@(posedge clk_equ)
begin
	if(UNIT_close && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,CMD_CLOZ,1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(UNIT_open && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,CMD_OPEN,1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(TxD_start && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,datain[15:0],1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(PL_start && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,pl[15:0],1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(PR_start && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,pr[15:0],1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(SYNC_start && ~WR_ctr) 
	begin
		datainbuf <= {1'b1,CMD_SYNC,1'b0};  
		WR_ctr <= 1'b1;    
	end
	else if(bincnt == 5'd18)
		WR_ctr <= 1'b0;
end*/
 
always@(posedge clk)
begin
	if(WR) 
	begin
		datainbuf <= {1'b1,datain,1'b0};  
		//WR_ctr <= 1'b1; 
		bincnt <= 8'd0;		
	end
	else
	begin
		if(clk_equ)                            
		begin
			if(bincnt<8'd18)
			begin
				TXD_reg <= datainbuf>>bincnt; 
				bincnt <= bincnt + 8'd1;          
				TI <= 1'b0;
			end
			else
			begin
				TI <= 1'b1;
				TXD_reg <= 1'b1;
			end
		end  
	end
end

assign TXD = TXD_reg; 
assign BUSY = ~TI;	 

endmodule 
