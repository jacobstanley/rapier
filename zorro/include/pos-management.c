// Functions to manage positions
// *******************************************************

void short_converted_usd_pair()
{
	if(quote_currency() == DENOM_CCY)
	{
		enterShort();
	}
		
	else if(base_currency() == DENOM_CCY)
	{
		enterLong();
	}
}

void long_converted_usd_pair()
{
	if(quote_currency() == DENOM_CCY)
	{
		enterLong();
	}
		
	else if(base_currency() == DENOM_CCY)
	{
		enterShort();
	}	
}

void adjust_usd_position(int current_lots, int target_lots)
/*Adjust positions from current_lots into target_lots considering position of USD in base/quote. */
{	
	int diff_lots = target_lots - current_lots;
	Lots = abs(diff_lots);

	if(!is(LookBack))
		print(TO_LOG, "\n\nAsset %s\nNew exposure is %d with formerly %d, diff is %d", Asset, target_lots, current_lots, diff_lots);
	
	if (diff_lots > 0) 
	{
		long_converted_usd_pair();
	} 
	else if (diff_lots < 0)
	{
		short_converted_usd_pair();
	}
}

int get_signed_open_lots()
/*Calculate the signed number of lots currently open with the currently selected asset (and algo), consdering DENOM_CCY.
Example:
* DENOM_CCY = USD
* 1 lot long of AUD/USD:  open_lots = 1
* 1 lot short of AUD/USD: open_lots = -1
* 1 lot long of USD/CAD:  open_lots = -1 (we are short CAD/USD)
* 1 lot short of USD/CAD: open lots = 1 (we are long CAD/USD)
*/
{
	int open_lots = 0;
	for(current_trades)  // cycles through all open and pending trades with the current asset and algo
	{
		if(TradeIsOpen and !TradeIsPhantom)
			open_lots += (-2 * TradeIsShort + 1) * TradeLots;
	}
	
	if(quote_currency() == DENOM_CCY)
		return open_lots;
	
	else if(base_currency() == DENOM_CCY)
		return -open_lots;
	
	else
		return open_lots; //DENOM_CCY is not in pair
}


var lots_to_usd_exposure(int num_lots)
/*For a given number of lots, calculate the equivalent usd exposure (well, DENOM_CCY exposure...)*/
{
	if(quote_currency() == DENOM_CCY)
		return num_lots*LotAmount*priceClose();
	
	else if(base_currency() == DENOM_CCY)
		return num_lots*LotAmount;
	else
	{
		/*
		store current asset
		switch to base/USD or USD/base
		get last price
		convert lots of base/quote to usd exposure
		*/
		string current_asset = Asset;
		if(!strcmp(strmid(Asset, 0, 3), "AUD") || !strcmp(strmid(Asset, 0, 3), "NZD") || !strcmp(strmid(Asset, 0, 3), "EUR")|| !strcmp(strmid(Asset, 0, 3), "GBP"))
		{
			asset(strf("%s/USD", strmid(Asset, 0, 3)));
			var last_price = priceClose();
			asset(current_asset);
			return num_lots*LotAmount*last_price;			
		}
		else if(!strcmp(strmid(Asset, 0, 3), "CAD") || !strcmp(strmid(Asset, 0, 3), "JPY") || !strcmp(strmid(Asset, 0, 3), "CHF"))
		{
			asset(strf("USD/%s", strmid(Asset, 0, 3)));
			var last_price = priceClose();
			asset(current_asset);
			return num_lots*LotAmount/last_price;			
		}
		else
		{
			printf("\nLots to USD conversion not implemented for %s", Asset);
			return 0;
		}
	}
	
	return 0;
}
var target_lots_from_base_lots(var base_lots)
/*Divide by ccy pair price in case quote ccy == denom ccy to arrive at correct number of lots */
{
	if(quote_currency() == DENOM_CCY)
		return base_lots /= priceClose();
	
	return base_lots;
}

int round_num_lots(var raw_num_lots)
/*Convert a fractional raw_num_lots to the best integer number of lots.*/
{
	int new_lots;
	if ( raw_num_lots > 0 && raw_num_lots < 1 ) 
	{
		return 1;
	}
	else if ( raw_num_lots < 0 && raw_num_lots > -1) 
	{
		return -1;
	} 
	else 
	{
		return round(raw_num_lots);
	}
	
}

// Functions to convert instruments to a common convention
// *******************************************************

vars getPriceSeries()
{
	if(quote_currency() == DENOM_CCY)
		{
			return series(priceClose());	
		}
	else if(base_currency() == DENOM_CCY)
		{
			return series(1./priceClose());	
		}
	return -1;
}

var convert_price(var price)
{
	if(quote_currency() == DENOM_CCY)
		{
			return price;	
		}
	else if(base_currency() == DENOM_CCY)
		{
			return 1./price;	
		}
	return -1;
	
}