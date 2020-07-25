//+------------------------------------------------------------------+
//|                                          Zorro.mq4/mq5 for MT4/5 |
//|                        Copyright 2015,2019 oP group Germany GmbH |
//|                                         http://zorro-project.com |
//+------------------------------------------------------------------+

#property copyright "Copyright 2015,2018 oP group Germany GmbH"
#property link      "http://zorro-project.com"

#define VERSION 2.06
//#define EXAMINE	"EURUSD"
//#define TESTMODE	// drag the price around for test purposes^
//#define OUTLIERTEST
//#define TRADETEST   // log all trade statuses
#define MSTIMER	   100   // timer for the main loop, ms
#define LIMIT_TIME   (11*60)  // wait time for pending trades, seconds
//#define LOG_ACCOUNT  5  // print the account values every x minutes

/////////////////////////////////////////////////////////////////////
#ifdef __MQL5__
#include "zmq4.mqh"
#import "ZorroMT64.ex5"
#else
#include <stdlib.mqh>
#import "ZorroMT4.ex4"
#endif
   int ZorroInit();
   int ZorroExit();
   int ZorroRequest(double& arr[],bool Update);
   string ZorroString();
   void ZorroRespond(int cmd,double& arr[]);
#import


// Zorro commands
#define CMD_COMMENT	160
#define CMD_PRINT		161
#define CMD_ASSET		170
#define CMD_HISTORY	172
#define CMD_TICK		173
#define CMD_BUY		174
#define CMD_TRADE		175
#define CMD_SELL		176
#define CMD_BALANCE	177
#define CMD_STOP		178
#define CMD_BCOMMAND	179

// brokerCommand types
#define SET_SLIPPAGE	129 // Max adverse slippage for buy orders
#define SET_MAGIC		130 // Magic number for trades
#define SET_ORDERTEXT 131 // order comment
#define SET_LIMIT		135 // set limit price for entry limit orders

#define SEND_PRICE	181 // 180..189: string commands
#define SEND_NTP		182 
#define PLOT_STRING	188 
#define SET_MODE		190

#define PLOT_REMOVE	260
#define PLOT_REMOVEALL 261

#define PLOT_HLINE	280 // 280..289: arr[5] commands
#define PLOT_TEXT		281
#define PLOT_MOVE		282
#define SEND_LINE		285

///////////////////////////////////////////////////////////////////////////////
int g_Slippage = 100; // price slippage tolerance in pips
int g_Magic = 0;	// magic number when nonzero
double g_Limit = 0;  // order limit when nonzero
uint g_UpdateTime = 200;
uint g_PlotNumber = 1;
string g_PlotName = "";
uint g_Testline = 0;
bool g_Running = false;
string g_OrderText = "Zorro";

#define var	double
var PIP = 0;
var PIPCost = 0;
var LotAmount = 0;

string assetFix(string Asset)
{
// detect and add Forex suffix from the chart window
	string s = Symbol();
	int ls = StringLen(s);
	int la = StringLen(Asset);
	int l = StringGetChar(Asset,la-1);
	if(ls > 6 && la == 6 && l <= 'Y' && l >= 'B') { // RUB .. JPY
		s = StringSubstr(s,6,ls-6);
#ifdef __MQL5__
      StringAdd(Asset,s);
#else
		Asset = StringConcatenate(Asset,s); 
#endif
      static bool once = true;
      if(once) {
         Print("Adding \"",s,"\" to asset names");
         once = false;
      }
	}
	return(Asset);
}

int orderTicketFix(datetime OpenTime,double OpenPrice) 
{
	int i;
	for(i=1; i<=OrdersTotal(); i++)       
		if(OrderSelect(i-1,SELECT_BY_POS,MODE_TRADES) == true )          
			if(OrderOpenTime() == OpenTime && OrderOpenPrice()== OpenPrice )              
				return(OrderTicket());                                       
	return(-1);  // no ticket found 
}

/////////////////////////////////////////////////////////////////////////////////
// preload the asset from the MT4 server
int assetLoad(string Asset,int tf,int shift)
{
   for(int i=0; ; i++) {
      int bars = iBars(Asset,tf);
      if(bars > 0 && shift > 1) {
			double c = iClose(Asset,tf,shift-1);
			if(c > 0)
				return(bars);
		}
      else if(bars > 0)
         return(bars);

      Print("Accessing ",Asset," from MT4 server - attempt ",i+1);
		int error = GetLastError();
      if(i < 4 || (i < 40 && error == 4066)) { // ERR_HISTORY_WILL_UPDATED
         Sleep(1000); 
      } else
         return(0); 
   }
   return(0); 
}

// generate the name of a trade
string tradeName(string Asset,int op,int id)
{
	string ls = "L";
	if(op == OP_SELL) ls = "S";
#ifdef __MQL5__
	static string Name;
	StringConcatenate(Name,"[",Asset,":",ls,id%10000,"]");
	return Name;
#else
	return(StringConcatenate("[",Asset,":",ls,id%10000,"]"));
#endif
}

int pointFactor(string Asset)
{
	int DigitSize = (int)MarketInfo(Asset,MODE_DIGITS); // correction for brokers with 5 or 6 digits
	if(DigitSize == 3 || DigitSize == 5) 
		return(10);
	else if(DigitSize == 6) 
		return(100);
	else
		return(1);
}

double lotSize(string Asset)
{
   double LotSize = MarketInfo(Asset,MODE_LOTSIZE);
   double LotMin = MarketInfo(Asset,MODE_MINLOT);
   //Print("LotSize ",LotSize," LotMin ",LotMin);
   if(LotSize*LotMin < 1) // LotAmount < 1
   	return LotMin; // vol = lots
   else
   	return 1./LotSize; // vol = contracts
}

/////////////////////////////////////////////////////////////////
#ifndef __MQL5__

color toColor(int rgb)
{
	int r = (rgb>>16)&255;
	int g = (rgb>>8)&255;
	int b = rgb&255;
	return (b<<16) + (g<<8) + r;
}

// align object left on chart
datetime align()
{
	datetime t;
	double p;
	int n;
	ChartXYToTimePrice(0,10,0,n,t,p);
	return t;
}

string setPlot(int Number)
{
	return g_PlotName = "ZorroGraph"+Number;
}

string newPlot(int Inc = 1)
{
	g_PlotNumber += Inc;
	return setPlot(g_PlotNumber);
}

int lineCreate(int n,double pos,color c,int w = 1,int s = 0)
{
	if(n == 0) {
		ObjectCreate(newPlot(),OBJ_HLINE,0,0,pos);
		ObjectSetInteger(0,g_PlotName,OBJPROP_COLOR,c);
		ObjectSetInteger(0,g_PlotName,OBJPROP_WIDTH,w);
		ObjectSetInteger(0,g_PlotName,OBJPROP_STYLE,s);
		ObjectSetString(0,g_PlotName,OBJPROP_TOOLTIP,g_PlotName);
		ObjectSetInteger(0,g_PlotName,OBJPROP_TIMEFRAMES,OBJ_ALL_PERIODS);
	} else {
		ObjectSet(setPlot(n),OBJPROP_PRICE1,pos);
		ObjectSetInteger(0,g_PlotName,OBJPROP_COLOR,c);
	}
	return g_PlotNumber; 
}

var lineGet(int n)
{
	return ObjectGet(setPlot(n),OBJPROP_PRICE1);
}

void lineDrag()
{
	g_Testline = lineCreate(g_Testline,Ask,clrCyan);
	ObjectSetInteger(0,g_PlotName,OBJPROP_SELECTABLE,true); 
	ObjectSetInteger(0,g_PlotName,OBJPROP_SELECTED,true); 
	ObjectSetString(0,g_PlotName,OBJPROP_TOOLTIP,"Drag Me");
}
#endif

////////////////////////////////////////////////////////////////////////
int run(int mode)
{
	static uint count = 0, loops = 0, ticks = 0;
	string Asset;
	int err,ticket;
	color C;
	double Price,Vol,Factor;
#ifdef EXAMINE
	if(0 == loops) {
		Asset = EXAMINE;
		double dLotFactor = MarketInfo(Asset,MODE_MINLOT); // correction for different lot scale
		double dFactor = pointFactor(Asset); // correction for brokers with 5 digits
		PrintFormat("Examine %s  Price %.5f  Spread %.5f",
			Asset,MarketInfo(Asset,MODE_ASK),MarketInfo(Asset,MODE_ASK)-MarketInfo(Asset,MODE_BID));
		PrintFormat("%s  Point %.5f  Digits %.0f  PIP %.5f",
			Asset,MarketInfo(Asset,MODE_POINT),MarketInfo(Asset,MODE_DIGITS),MarketInfo(Asset,MODE_POINT)*dFactor);
		//PrintFormat("%s  Tick %.5f  TickVal %.5f",
		//	Asset,MarketInfo(Asset,MODE_TICKSIZE),MarketInfo(Asset,MODE_TICKVALUE));
		PrintFormat("%s  LotMin %.2f  LotSize %.2f  LotAmount %.2f",
			Asset,dLotFactor,MarketInfo(Asset,MODE_LOTSIZE),MarketInfo(Asset,MODE_LOTSIZE)*dLotFactor);
		PrintFormat("%s  PipCost %.5f  MarginCost %.2f",
			Asset,MarketInfo(Asset,MODE_TICKVALUE) * dLotFactor * dFactor,
			MarketInfo(Asset,MODE_MARGINREQUIRED) * dLotFactor);
	}
#endif
#ifdef LOG_ACCOUNT
   static uint LastLog = 0;
   if(GetTickCount() > LastLog + LOG_ACCOUNT*60*1000) {
      LastLog = GetTickCount();
   	PrintFormat("\nBal %.2f  Equ %.2f  Mrg %.2f",
   	   AccountBalance(),AccountEquity(),AccountMargin());
   }
#endif

	bool Update = true;
	if(Update == false && (ticks > GetTickCount() || GetTickCount() - ticks > g_UpdateTime))
	{
		Update = true;
		ticks = GetTickCount();
	}
	count++;
   //if(MarketInfo(Symbol(),MODE_TRADEALLOWED) == 0)
   //   Comment("Market closed");

   static double arr[10];
   int cmd = 1;
   while(cmd != 0) {
   	loops++;
		//Comment("Runs ",count," Loops ",loops); 
      cmd = ZorroRequest(arr,Update);
      switch(cmd) 
      {
         case CMD_COMMENT: 
         	Update = true;
            Comment(ZorroString()); 
            Print(ZorroString()); 
            break;  
                   
         case CMD_PRINT: 
         	Update = true;
            Print(ZorroString()); 
            break;         

         case CMD_ASSET: {
         	if(!Update) break;
         	Asset = assetFix(ZorroString());
         	arr[0] = MarketInfo(Asset,MODE_ASK); // price
      		if(arr[0] == 0) {
      			err = GetLastError();
      			if(err == ERR_UNKNOWN_SYMBOL) {
      				SymbolSelect(Asset,1);
		         	arr[0] = MarketInfo(Asset,MODE_ASK); // price
		         	static int warned = 0;
		         	if(arr[0] == 0 && warned++ <= 1)
							Print(Asset," unknown (",GetLastError(),")");	         	
      			} else {
						Print(Asset," data missing (",err,")");
		            ZorroRespond(cmd,arr);
	            	break;
	            }
				}
				//else Comment(asset + " " + arr[0]);
         	arr[1] = arr[0] - MarketInfo(Asset,MODE_BID); // spread
#ifdef TESTMODE
				if(g_Testline)	arr[0] = lineGet(g_Testline);
#endif
#ifdef OUTLIERTEST
            double dLow = MarketInfo(Asset,MODE_LOW);
            double dHigh = MarketInfo(Asset,MODE_HIGH);
            if(arr[0] > 1.25*dLow || arr[0] < 0.75*dHigh)
               Print(Asset," Outlier: ",arr[0]);
#endif 
         	arr[2] = 0; // Volume
				double LotFactor = MarketInfo(Asset,MODE_MINLOT); // correction for different lot scale
        		Factor = pointFactor(Asset); // correction for brokers with 5 digits
         	arr[3] = PIP = MarketInfo(Asset,MODE_POINT) * Factor; // pip
         	arr[4] = PIPCost = MarketInfo(Asset,MODE_TICKVALUE) * LotFactor * Factor; // pipcost
         	arr[5] = LotAmount = MarketInfo(Asset,MODE_LOTSIZE) * LotFactor;
         	if(LotAmount == 0. && arr[0] > 0.)
            	Print("Asset ",Asset," Lot ",MarketInfo(Asset,MODE_LOTSIZE)," Min ",LotFactor);
         	arr[6] = MarketInfo(Asset,MODE_MARGINREQUIRED) * LotFactor; // margin
         	arr[7] = MarketInfo(Asset,MODE_SWAPLONG);
         	arr[8] = MarketInfo(Asset,MODE_SWAPSHORT);
				if(MarketInfo(Asset,MODE_SWAPTYPE) == 0.) {
					arr[7] *= arr[4];
					arr[8] *= arr[4];
				}
         	arr[9] = iVolume(Asset,PERIOD_M15,0)/15.;
            ZorroRespond(cmd,arr);
            break;
            }
         case CMD_HISTORY: {
         	Asset = assetFix(ZorroString());
            int timeframe = (int)arr[0];
            int start = (int)arr[1];
            int end = (int)arr[2];
            int nTicks = (int)arr[3];
            int shift1 = MathMax(0,iBarShift(Asset,timeframe,start,false));
            int shift = MathMax(0,iBarShift(Asset,timeframe,end,false));
            int num = 0;
         	if(!SymbolSelect(Asset,1)
					|| !assetLoad(Asset,timeframe,shift)
         	   || !assetLoad(Asset,timeframe,shift1)) {
					Print(Asset," history missing (",GetLastError(),")");
       		} else {
       			for(;num < nTicks;shift++) {
               	arr[0] = iOpen(Asset,timeframe,shift);
               	arr[1] = iHigh(Asset,timeframe,shift);            
               	arr[2] = iLow(Asset,timeframe,shift);
               	arr[3] = iClose(Asset,timeframe,shift);
               	arr[5] = 0;
               	arr[6] = iVolume(Asset,PERIOD_M15,shift)/15.; 
               	int time = (int)iTime(Asset,timeframe,shift);
               	arr[4] = time + 60*timeframe; // assume tick start time
               	if(arr[0] == 0 || time == 0 || time < start) {
               		Print(Asset," end of history at bar ",shift);
               		break; 
               	}
               	ZorroRespond(CMD_TICK,arr);
               	num++;
            	}
            	if(num > 0) 
            		Print("Send ",num," ",Asset," bars at ",
            			TimeToStr(end,TIME_DATE|TIME_MINUTES),
            			" (",shift-num," - ",shift,")");
            }
            arr[0] = num;
            ZorroRespond(cmd,arr);
            break;
            }
         case CMD_BALANCE:
           	if(!Update) break;
         	arr[0] = AccountBalance();
	   		arr[1] = AccountEquity() - arr[0]; 
   			arr[2] = AccountMargin();
            if(MarketInfo(Symbol(),MODE_TRADEALLOWED) == 0)
   			   arr[3] = 0;
   		   else
   			   arr[3] = (double)TimeCurrent();
   			ZorroRespond(cmd,arr);
            break;

         case CMD_BUY: {
            int op; 
            color c;
            double price, stop;
            datetime expiration = 0;
         	Asset = assetFix(ZorroString());
            if(arr[0] > 0.) {
            	op = OP_BUY;
            	c = MediumBlue;
            	price = MarketInfo(Asset,MODE_ASK);
            	if(g_Limit > 0.) {
           	      op = OP_BUYLIMIT;
            	   price = MathMin(price,g_Limit);
            	   expiration = TimeCurrent()+LIMIT_TIME; // fails with less than 660
            	}
            	if(arr[1] <= 0.)
            		stop = 0.;
            	else
            		stop = price - arr[1];
            } else {
               op = OP_SELL;
            	c = RoyalBlue;
               price = MarketInfo(Asset,MODE_BID);
            	if(g_Limit > 0.) {
           	      op = OP_SELLLIMIT;
            	   price = MathMax(price,g_Limit);
            	   expiration = TimeCurrent()+LIMIT_TIME;
            	}
            	if(arr[1] <= 0.)
            		stop = 0.;
            	else
	            	stop = price + arr[1];
            }
				Vol = lotSize(Asset)*MathAbs(arr[0]);
        		Factor = pointFactor(Asset); // correction for brokers with 5 digits
            int magic = (int)arr[3];
            if(g_Magic > 0) magic = g_Magic;
            Print("Open ",Asset," ",Vol,"@",price," stop ",stop," slp ",g_Slippage," magic ",magic," factor ",Factor);
            arr[0] = OrderSend(Asset,op,Vol,price,(int)(g_Slippage*Factor),stop,0,g_OrderText,magic,expiration,c);
            if(arr[0] < 0.) {
             	err = GetLastError();
            	Print("Order failed - ", ErrorDescription(err));
            	arr[0] = 0.;
            } else {
            	Comment("Trade ",tradeName(Asset,op,(int)arr[0])," entered");
            	Print("Ticket ",arr[0]," Total ",OrdersTotal());
            	//if(PositionSelectByTicket((int)arr[0]))
               //	Print("Symbol ",PositionGetString(POSITION_SYMBOL));
               //if(OrderSelect((int)arr[0],SELECT_BY_TICKET))
           	   //for(int i=0;i<OrdersTotal();i++)
               //	if(OrderSelect(i,SELECT_BY_POS,MODE_TRADES))
               //		Print("Lots ",OrderLots()," Symbol ",OrderSymbol());
            }
            if(g_Limit > 0. || arr[0] == 0.)
               arr[1] = 0.; // not yet filled
            else
               arr[1] = price;
            g_Limit = 0;
            ZorroRespond(cmd,arr);
            break;
            }
         case CMD_TRADE: {
         	ticket = (int)arr[0]; 
         	arr[0] = 0;
         	if(OrderSelect(ticket,SELECT_BY_TICKET)) {
         		double amount = OrderLots()/lotSize(OrderSymbol())+0.1;
         		if(OrderCloseTime() != 0) amount = -amount;
         		arr[0] = amount;
         		if(OrderType() == OP_BUY || OrderType() == OP_SELL) // not pending anymore?
         		   arr[1] = OrderOpenPrice();
         		else
         		   arr[1] = 0.;
         		arr[2] = OrderClosePrice();
         		arr[3] = OrderSwap();
         		arr[4] = OrderProfit();
#ifdef TRADETEST
	            Print("Trade ",ticket," amt ",arr[0]," op ",arr[1]," cl ",arr[2]," com ",arr[3]," pr ",arr[4]);
#endif
         	} else
	            Print("Trade ",ticket," not found in ",OrdersTotal()," trades");
            ZorroRespond(cmd,arr);
            break;
            }
         case CMD_STOP: {
         	ticket = (int)arr[0];
         	arr[0] = 0;
         	if(OrderSelect(ticket,SELECT_BY_TICKET)) {
         		if(OrderModify(ticket,OrderOpenPrice(),arr[1],0,0,Blue))
  		         	arr[0] = 1;
         	}
            ZorroRespond(cmd,arr);
            break;         
            }
         case CMD_SELL: {
         	ticket = (int)arr[0];
     			arr[0] = 0;
            C = MediumBlue;
         	if(OrderSelect(ticket,SELECT_BY_TICKET)) {
         	   Asset = OrderSymbol();
					Vol = lotSize(Asset)*MathAbs(arr[1]);
         		Price = OrderClosePrice();
               //Print("Symbol ",Asset," LotSize ",LotSize," Price", Price);
         		if(OrderType() == OP_SELL)
            		C = RoyalBlue;
	            Print("Close ",tradeName(OrderSymbol(),OrderType(),ticket)," ",Vol,"@",Price," => ",OrderProfit(),OrderExpiration());
	            int total = OrdersTotal();
	            double OpenPrice = OrderOpenPrice();
	            datetime OpenTime = OrderOpenTime();
            	if(OrderCloseTime() > 0)
         			arr[0] = OrderTicket(); // already closed by stop loss
         		else if(OrderType() != OP_BUY && OrderType() != OP_SELL) { // still pending?
	         		if(OrderDelete(ticket,C)) {
	 		           	Comment(tradeName(OrderSymbol(),OrderType(),ticket)," deleted");
	 		           	arr[0] = ticket;
	 		         }
         		} else if(OrderClose(ticket,Vol,Price,g_Slippage*pointFactor(Asset),C)) {
            		if(total == OrdersTotal()) { // same number of orders -> only partially closed?
            			int NewTicket = orderTicketFix(OpenTime,OpenPrice);
#ifndef __MQL5__ // OrderTicket() not supported by MQL5
            			if(NewTicket > 0) {
            				ticket = NewTicket;
	            			Comment(tradeName(OrderSymbol(),OrderType(),ticket)," partial");
	            		}
#endif
			            Print("Remaining ",tradeName(OrderSymbol(),OrderType(),NewTicket)," ",OrderLots(),"@",Price);
	            	} else
 		           		Comment(tradeName(OrderSymbol(),OrderType(),ticket)," closed");
            		arr[0] = ticket;
            		// arr[1] = fill = unchanges
            		arr[2] = OrderClosePrice();
            		arr[3] = OrderSwap();
            		arr[4] = OrderProfit();
         		} else {
            		Print("Close at ",Price," failed - ", ErrorDescription(GetLastError()));
         			arr[0] = 0;
            	}
         	}
            ZorroRespond(cmd,arr);
            break;         
            }
         case CMD_BCOMMAND: {
         	int command = (int)arr[0];
         	//Print("Command ",command,": ",arr[1]," ",arr[2]," ",arr[3]);
         	switch(command) {
         		case SET_SLIPPAGE:
	         		g_Slippage = (int)arr[1]; 
	         		//Comment("Slippage",g_Slippage);
	         		arr[0] = 1; break;
         		case SET_MAGIC:
         			g_Magic = (int)arr[1]; 
         			arr[0] = 1; break;
         		case SET_LIMIT:
         			g_Limit = arr[1];
         			arr[0] = g_Limit; break;
         		case SET_ORDERTEXT:
         			g_OrderText = ZorroString();
         			//Print("Ordertext",g_OrderText);
         			arr[0] = 1; break;
#ifndef __MQL5__
 	       		case PLOT_STRING:
						arr[0] = ObjectSetString(0,g_PlotName,OBJPROP_TEXT,ZorroString());
 	       			newPlot(2);
						break;
 	       		case PLOT_HLINE:
 	       			newPlot();
						ObjectCreate(g_PlotName,OBJ_HLINE,0,arr[1],arr[2]);
						ObjectSetInteger(0,g_PlotName,OBJPROP_COLOR,toColor(arr[3]));
						ObjectSetInteger(0,g_PlotName,OBJPROP_WIDTH,(int)arr[4]);
						ObjectSetInteger(0,g_PlotName,OBJPROP_STYLE,(int)arr[5]);
						ObjectSetInteger(0,g_PlotName,OBJPROP_TIMEFRAMES,OBJ_ALL_PERIODS);
						arr[0] = g_PlotNumber; 
						break;
 	       		case PLOT_TEXT:
 	       			newPlot();
 	       			if(arr[1] == 0.)
 	       				arr[1] = align();
						ObjectCreate(g_PlotName,OBJ_TEXT,0,arr[1],arr[2]);
						ObjectSetText(g_PlotName,g_PlotName,arr[4],"Arial",toColor(arr[3]));
						ObjectSetInteger(0,g_PlotName,OBJPROP_ANCHOR,ANCHOR_LEFT_LOWER);
						ObjectSetInteger(0,g_PlotName,OBJPROP_TIMEFRAMES,OBJ_ALL_PERIODS);
						arr[0] = g_PlotNumber; 
						break;
 	       		case PLOT_REMOVE:
 	       			setPlot((int)arr[1]);
						arr[0] = ObjectDelete(g_PlotName);
						break;
 	       		case PLOT_REMOVEALL:
						ObjectsDeleteAll();
#ifdef TESTMODE
					   lineDrag();
#endif
						break;
 	       		case PLOT_MOVE:
 	       			setPlot((int)arr[1]);
 	       			if(arr[2] == 0.)
 	       				arr[2] = align();
 	       			ObjectSet(g_PlotName,OBJPROP_TIME1,arr[2]);
						arr[0] = ObjectSet(g_PlotName,OBJPROP_PRICE1,arr[3]);
						break;
#endif
               default: // all command numbers are passed to the bridge
                  arr[0] = 0; // return 0 if not processed
                  break;
				}
            ZorroRespond(cmd,arr);
            break; 
         }
      }
   }
   return(0);
}

void OnTick()
{
	run(1);
}

void OnTimer()
{
	run(0);
}

int OnInit()
{
	if(!g_Running) {
	   if(ZorroInit() <= 0)
	      return(INIT_FAILED);
		g_Running = true;
		EventSetMillisecondTimer(MSTIMER);
		Print("Zorro EA ",VERSION," - Timer ",MSTIMER," ms");
		Comment("Controlled by Zorro EA ",VERSION);
	}
	return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason)
{
	if(UninitializeReason() != REASON_CHARTCHANGE) {
#ifndef __MQL5__
		ObjectsDeleteAll(); 
#endif
		g_Running = false;
		ZorroExit();
	}
}
