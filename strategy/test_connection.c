/*
Test broker connection
*/
function run()
{
    BarPeriod = 1;
    LookBack = 0;

    // set Command[0] with -i option
	total_usd_exposure = Command[0];
	if(is(INITRUN)) {
		printf("\n### USD exposure: %d", total_usd_exposure);
	}
    
    printf("\n%02i-%02i-%04i %02i:%02i %s %.5f, %.5f, %.5f, %.5f",
    day(), month(), year(), hour(), minute(), Asset, priceOpen(), priceHigh(), priceLow(), priceClose());
}
