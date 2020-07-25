#include <default.c>

void scale_demean(var *arr_to_scale, var *target_array, int length)
{
	var mean = Moment(arr_to_scale, length, 1);
	int i;
	for(i=0; i<length; i++)
	{
		 target_array[i] = (arr_to_scale[i] - mean); 
	}
}

void scale_minmax(var *arr_to_scale, var *target_array, int length)
{
   // get min and max of array
   var array_min = arr_to_scale[0];
   var array_max = arr_to_scale[0];
	int i;
   for(i=0; i<length; i++)
   {
		if(arr_to_scale[i] < array_min)
			array_min = arr_to_scale[i];
		else if(arr_to_scale[i] > array_max)
         array_max = arr_to_scale[i];
   }

   // scale array by min-max
	for(i=0; i<length; i++)
   {
		target_array[i] = 2*(arr_to_scale[i] - array_min)/(array_max - array_min) - 1;
   }
}

void scale_sum_abs(var *arr_to_scale, var *target_array, int length)
{
	int i;
	var sumabs = 0;
	for(i=0; i<length; i++)
	{
		sumabs += abs(arr_to_scale[i]);
	}
	
	for(i=0; i<length; i++)
	{
		if(sumabs == 0)
			target_array[i] = 0;
		else
			target_array[i] = arr_to_scale[i]/sumabs;
	}
	
}

void scale_zscore(var *arr_to_scale, var *target_array, int length)
{
	var mean, stdev;
	mean = Moment(arr_to_scale, length, 1);
	stdev = sqrt(Moment(arr_to_scale, length, 2));
	
	int i;
	for(i=0; i<length; i++)
	{
		 target_array[i] = (arr_to_scale[i] - mean) / stdev; 
	}
}

void scale_zscore_norm(var *arr_to_scale, var *target_array, int length)
{
	vars zscores = malloc( length * sizeof(var) );
	if ( zscores == NULL ) {
		printf("\n!!! Error allocating array in 'scale_zscore_norm',\nreturning original factors !!!");
		memcpy( target_array, arr_to_scale, length * sizeof(var) );
		return;
	}
	
	scale_zscore(arr_to_scale, zscores, length);
	// Return standard normal cumulative distribution from zscore. (Equivalent to NORMSDIST in Excel.)
	int i;
	for(i=0; i<length; i++)
	{
		 target_array[i] = (cdf(zscores[i]) - 0.5) * 2;
	}
	free(zscores);
}

void scale_ernie(var *arr_to_scale, var *target_array, int length)
{
	// Calculate sum of absolute differences from mean
	var mean = Moment(arr_to_scale, length, 1);
	var absdiff = 0.0;
	int i;
	for(i=0; i<length; i++)
	{
		absdiff += abs(arr_to_scale[i] - mean);
	}
	for(i=0; i<length; i++)
	{
		target_array[i] = (arr_to_scale[i] - mean) / absdiff;
	}
}