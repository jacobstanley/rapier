/* 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Zorro variable swap functions: instructions for use
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* sync resilio interest rate data into market data history folder
* save this file in the include directory of your Zorro installation
* to add this file's functionality to a script, add the line #include <variable_swap.c> at the top of your script. 
* this script requries that the interest rate datasets loaded from csv files be given a unique hash value corresponding to the name of the relevant currency. See example.
* if you want to run a simulation with variable swap, add the line #define VARIABLE_SWAP to your script. Comment this out to disable variable swap simulation.
* define your asset list (ASSET_LIST), account currency (ACCT_CCY) and broker swap markup (BROKER_FEE) in your script. See example.
* since the function in variable_swap.c needs ACCT_CCY make sure this is defined before the line #include <variable_swap.c>
* this only works for FOREX assets. For non-FOREX assets used in your script, the RollLong and RollShort values in the Assets file are used. 

Stuff to note and possibly fix:
~~~~~~~~~~~~~~~~~~~~~~~
- will get different results for different account currencies. Suggest using USD-denominated accounts for simulation in the bootcamp.
- we adjust roll by floating exchange rate - this is more accurate than what Zorro does with it's PIPCost variable, which is assumed static throughout the simulation (although we could modify this too)
- have performed rudimentary testing to ensure that assets are being swapped as expected using the various string manipulation tools. 
- calculated rolls have been unit-tested using "test_variable_swap.c"
- sometimes, according to dataParse() an interest rate value fails to be read from file, but in most cases it appears that this is incorrect and the data does get read in - haven't gotten to the bottom of this bug yet. 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Explanation of roll calculations (courtesy @kujo):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Consider this example:

We buy 10,000 units EUR/USD @1.15. It means that we bought 10,000 EUR and sold 11,500 USD.
Financing for EUR per day: 10,000EUR/365*EUR_rate
Financing for USD per day: 11,500USD/365*USD_rate = 10,000EUR/365*USD_rate
RollLong is 10,000EUR/365*EUR_rate -  10,000EUR/365*USD_rate = 10,000EUR / 365 * (EUR_rate - USD_rate)

#### So, the result is in EUR (base CCY) not USD (quote CCY). ####

RollLong in USD will be: 11,500USD / 365 * (EUR_rate - USD_rate) = 10,000EUR / 365 * (EUR_rate - USD_rate) * EUR/USD_quote

This leads to the general formula for RollLong for BASE_CCY/QUOTE_CCY in ACCT_CCY: 
10,000BASE_CCY / 365 * (BASE_CCY_rate - QUOTE_CCY_rate) * BASE_CCY/ACCT_CCY

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CHANGELOG
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2019-02-08: add check of timesteamp of last ir data update (robotkris)
2019-02-10: changed currency pair lookup strategy, general optimisations (DerBob)

*/

#include <contract.c>

string base_currency()
{
	/* return the base currency of the currently selected asset as a temporary string */
	return strmid(Asset, 0, 3);
}

string quote_currency()
{
	/* return the quote currency of the currently selected asset as a temporary string */
	return strmid(Asset, 4, 3);
}

var calculate_roll_long(var base_ir, var quote_ir, var broker_fee)
{
    /*Calculates Zorro roll long in units of base currency*/
    var ird = (base_ir - quote_ir)/100;
    return 10000*ird/365 - broker_fee;
}

var calculate_roll_short(var base_ir, var quote_ir, var broker_fee)
{
    /*Calculates Zorro roll short in units of base currency*/
    var ird = (quote_ir - base_ir)/100;
    return 10000*ird/365 - broker_fee;
}

void get_file_path(string path, string currency) {
	#ifdef IR_PATH
		strcpy(path, IR_PATH);
		strcat(path, "\\");
		strcat(path, currency);
	#else
		strcpy(path, currency);
	#endif
}

#ifdef VARIABLE_SWAP

var base_to_acct_currency_conversion_factor()
{
	string base_ccy = base_currency();
	
	/* Calculate conversion factor for converting swap in base currency to account currency */
	if(assetType(Asset) != FOREX)
		// TODO: implement variable swap for non-FX assets, if we can get data.
		return 1.;
	char current_asset[8]; 
	strcpy(current_asset, Asset); 
	if(strcmp(ACCT_CCY, base_ccy))  //account currency is different to base currency
	{
		string acct_base_pair = strf("%s/%s", ACCT_CCY, base_ccy);
		string base_acct_pair = strf("%s/%s", base_ccy, ACCT_CCY);
		
		// look through all loaded assets if a suitable pair can be found
		int j = 0; 
		while( Assets[j] ) {
			if (!strcmp(Assets[j], acct_base_pair)) {
				asset(acct_base_pair);
				// if we are here, the selected asset is ACCT_CCY/BASE_CCY - we need the inverse price.
				if(priceClose() != 0) {
					var conversion_factor = 1./priceClose();
					asset(current_asset);
					return conversion_factor;
				} else {
					asset(current_asset);
					return 1.; // if no price available, this will leave roll in terms of the base currency - not a disaster, but not accurate. 
				}
			} else if (!strcmp(Assets[j], base_acct_pair)) {
				asset(base_acct_pair);
				// if we are here, the selected asset is BASE_CCY/ACCT_CCY - we need the raw price
				if(priceClose() != 0) {
					var conversion_factor = priceClose();
					asset(current_asset);
					return conversion_factor;
				}
				else {
					asset(current_asset);
					return 1.; // if no price available, this will leave roll in terms of the base currency - not a disaster, but not accurate. 
				}
			}
			j++;
		}
		
		// no suitable pair was found in loaded assets
		printf("\nNo data for either %s or %s", acct_base_pair, base_acct_pair);
		return 1.; // if an issue was encountered, this will leave roll in terms of the base currency - not a disaster, but not accurate.
	}
	// if we are here, BASE_CCY and ACCT_CCY are the same
	return 1.;
}

void set_roll(var broker_fee)
/*
Adjusts RollLong and RollShort using historical central bank policy rate data stored in CSV files named by currency.
The adjustment includes accounting for fluctuations in the account currency in terms of the quote currency.
*/
{
	string base_ccy, quote_ccy;
	string base_ccy_path[200], quote_ccy_path[200];
	
	base_ccy = base_currency(); 
	quote_ccy = quote_currency();
	
	get_file_path(base_ccy_path, base_ccy);
	get_file_path(quote_ccy_path, quote_ccy);

	int base_handle = stridx(base_ccy);
	int quote_handle = stridx(quote_ccy);
	
	
	if(is(INITRUN))
	{
		if( dataFind(base_handle, 0) < 0 ) {
			if(!dataParse(base_handle, "%Y-%m-%d,f", base_ccy_path))
			{
				printf(strf("\ninterest rate not read for %s", base_ccy));
			}
		}
		
		if( dataFind(quote_handle, 0) < 0) {
			if(!dataParse(quote_handle, "%Y-%m-%d,f", quote_ccy_path))
			{
				printf(strf("\ninterest rate not read for %s", quote_ccy));
			}
		}
	}
	
	var base_ir = dataFromCSV(base_handle, "%Y-%m-%d,f", base_ccy_path, 1, 0);
	// check timestamp of last base currency ir update
	int base_data_day = ymd(dataFromCSV(base_handle, "%Y-%m-%d,f", base_ccy_path, 0, 0));
	// compare to this day
	int this_day = ymd(wdate());
	if(dmy(this_day) - dmy(base_data_day) > 0)
	{
		printf("\nWARNING: Last %s ir update was %0.f days ago", base_ccy, dmy(this_day) - dmy(base_data_day));
	}
	var quote_ir = dataFromCSV(quote_handle, "%Y-%m-%d,f", quote_ccy_path, 1, 0);
	// check timestamp of last quote currency ir update
	int quote_data_day = ymd(dataFromCSV(quote_handle, "%Y-%m-%d,f", quote_ccy_path, 0, 0));
	// compare to this day
	if(dmy(this_day) - dmy(quote_data_day) > 0)
	{
		printf("\nWARNING: Last %s ir update was %0.f days ago", quote_ccy, dmy(this_day) - dmy(base_data_day));
	}
	var base_roll_long = calculate_roll_long(base_ir,  quote_ir, broker_fee);  
	var base_roll_short = calculate_roll_short(base_ir,  quote_ir, broker_fee); 
	// adjust for account currency
	var conversion_factor = base_to_acct_currency_conversion_factor();
	RollLong = base_roll_long*conversion_factor;
	RollShort = base_roll_short*conversion_factor;
}
#else
var base_to_acct_currency_conversion_factor() {
	return 1;
}
void set_roll(var broker_fee){
	return;
}
#endif