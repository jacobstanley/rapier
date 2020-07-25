// Factor functions
//***********************

var momo_factor(vars prices, int days)
{
	//printf("\n%f days ago: %d", prices[days], days);
	return (prices[0] - prices[days]) / prices[days];
}

var sma_diff_factor(vars prices, int days)
{
	 vars long_sma = series(SMA(prices, days));
	 vars short_sma = series(SMA(prices, (int)(days * 0.1)));
	 return (short_sma[0] / long_sma[0]) - 1;
}

var price_to_sma_factor(vars prices, int days)
{
	vars sma = series(SMA(prices, days));
	return (prices[0] / sma[0]) - 1;
}

var price_to_ema_factor(vars prices, int days)
{
	vars ema = series(EMA(prices, days));
	return (prices[0] / ema[0]) - 1;
}

var sma_slope_factor(vars prices, int days)
{
	vars sma = series(SMA(prices, days));
	return (sma[0] / sma[1]) - 1;
}

var percent_rank_factor(vars prices, int days)
{
	return PercentRank(prices, days, prices[0]);
}

var zscore_factor(vars prices, int days)
{
	return zscore(prices[0], days);
	
}

var spearman_factor(vars prices, int days)
{
	return Spearman(prices, days);
}
