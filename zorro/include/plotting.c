void plot_algo_equity(int color)
/*plots equity curve for an algo in a new window*/
{
	var equity;
	int i;
	string Name;
	for(i=0; Name=Assets[i]; i++)
	{
		asset(Name);
		equity += EquityShort+EquityLong;
		
	}
	plot(Algo,equity,NEW|AVG|BARS,color);
}

void plot_algo_usd_exposure(int color)
/*plots usd exposure, in 1000's, for an algo in a new window*/
{
	var algo_exposure = 0;
	int i;
	string Name;
	for(i=0; Name=Assets[i]; i++)
	{
		asset(Name);
		var position = get_signed_open_lots();
		algo_exposure += lots_to_usd_exposure(position);
	}
	plot(strf("%s-Exp", Algo), algo_exposure/1000, NEW|BARS, color);
	
}

void plot_asset_usd_exposure(int color)
/*plots usd exposure, in 1000's, for an asset in a new window*/
{
	var asset_exposure = 0;
	int i;
	string Name;
	for(i=0; Name=Assets[i]; i++)
	{
		asset(Name);
		var position = get_signed_open_lots();
		asset_exposure += lots_to_usd_exposure(position);
		plot(strf("%s-Exp", Asset), asset_exposure/1000, NEW|BARS, color);
	}
}

void plot_algo_asset_position(int color)
/*plots position, in units of LotAmount, for all assets in an algo in new windows*/
{
	int i;
	string Name;
	for(i=0; Name=Assets[i]; i++)
	{
		asset(Name);
		var position = get_signed_open_lots();
		plot(strf("%s-%s", Algo, Asset), position, NEW|BARS, color);
	}	
	
}

void plot_asset_position_and_algo_exposure(int color_asset, color_algo)
/*plots position, in units of LotAmount, for all assets in an algo in new windows, 
AND usd exposure, in 1000's, for an algo in a new window
that is, combines plot_algo_usd_exposure() and plot_algo_asset_position()
*/
{
	var algo_exposure = 0;
	int i;
	string Name;
	for(i=0; Name=Assets[i]; i++)
	{
		asset(Name);
		var position = get_signed_open_lots();
		algo_exposure += lots_to_usd_exposure(position);
		plot(strf("%s-%s", Algo, Asset), position, NEW|BARS, color_asset);
	}
	plot(strf("%s-Exp", Algo), algo_exposure/1000, NEW|BARS, color_algo);
	
}

