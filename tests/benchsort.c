
/* https://github.com/cave7/sort/blob/master/quicksort.c */

#ifndef EM_PORT_API
#	if defined(__EMSCRIPTEN__)
#		include <emscripten.h>
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype EMSCRIPTEN_KEEPALIVE
#		else
#			define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE
#		endif
#	else
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype
#		else
#			define EM_PORT_API(rettype) rettype
#		endif
#	endif
#endif

int quicksort_r(int* a,int start,int end){
    if (start>=end) {
        return 0;
    }
    int pivot=a[end];
    int swp;
    //set a pointer to divide array into two parts
    //one part is smaller than pivot and another larger
    int pointer=start;
    int i;
    for (i=start; i<end; i++) {
        if (a[i]<pivot) {
            if (pointer!=i) {
                //swap a[i] with a[pointer]
                //a[pointer] behind larger than pivot
                swp=a[i];
                a[i]=a[pointer];
                a[pointer]=swp;
            }
            pointer++;
        }
    }
    //swap back pivot to proper position
    swp=a[end];
    a[end]=a[pointer];
    a[pointer]=swp;
    quicksort_r(a,start,pointer-1);
    quicksort_r(a,pointer+1,end);
    return 0;
}

int quicksort(int*a, int len){
    quicksort_r(a,0,len-1);
    return 0;
}


int a[1000]={4408,6681,316,481,4730,5125,6503,3788,3123,8609,2214,4547,7079,9285,3702,7255,3375,6577,9828,1876,3571,7951,1005,2681,5357,3164,7822,2994,6293,3558,1829,3436,2868,1341,4470,4135,3366,106,4156,4064,8928,6003,7445,139,5006,3859,7331,5160,4762,9351,7892,3760,9713,8616,691,4393,2759,5127,5526,8212,4698,1891,8535,567,9082,5819,357,7508,2800,2174,4849,5425,247,5227,4548,1650,6685,8770,6785,7838,5455,8102,8436,145,9283,1741,5482,9877,2760,6299,9377,5953,2478,436,2849,4639,1678,6920,6732,8588,4286,5305,4882,8268,8121,1570,1815,5670,2870,4760,5286,2936,8812,1141,7179,3700,6250,2547,979,6188,3319,5844,9229,4853,8117,3904,6200,3062,5817,6577,1136,4825,1849,9876,7852,3073,9757,3853,4225,8777,7649,2136,8806,8264,2823,2900,7573,8258,8297,2353,9900,3910,244,3906,7240,4276,6171,1332,4585,4294,1715,6010,8690,9671,5397,2929,6820,4634,5899,6755,4894,2705,2851,4278,5371,2891,5578,2393,8226,2548,8334,7593,1754,1031,2668,7387,2887,4383,2431,8003,9659,6192,1499,9723,2966,2808,1985,3165,4960,8943,8480,2670,4452,497,1491,8577,8152,9214,8967,2500,2942,9101,9414,8584,4694,3058,4076,14,5659,4456,5257,5509,4267,7460,3975,7373,7096,7570,6890,6245,3791,6690,813,3869,8777,2977,3504,4628,218,91,9003,5404,2759,793,4373,1710,5499,3378,3634,4964,7327,4859,6584,825,7187,4332,690,6852,5712,2914,3713,1085,52,8775,9248,2712,7683,3801,6654,8145,8376,1462,1820,2732,8446,4192,8397,9321,6404,8988,236,4350,6745,4278,2903,3326,2278,8611,6057,3228,5188,8354,8741,1066,9814,7265,7528,3596,7840,9365,2011,4143,7554,3549,7382,6408,8163,984,4498,7389,1331,1268,8995,5186,2387,9096,1039,3700,9154,8685,3728,6151,4574,5009,4964,9325,2733,6262,7027,1070,996,4861,3481,595,991,6400,4345,5182,3676,1784,6734,7609,4413,1071,1895,8451,8309,573,9195,4995,5073,1264,5188,3345,5257,1196,849,477,1507,4922,3700,5155,1298,5086,6010,2856,5658,9006,9198,9569,3139,2666,3019,7479,9968,7733,8575,8155,5689,7342,8153,4628,4515,5370,287,8201,6430,2655,9403,4955,9016,9646,1895,4072,8657,3389,987,1380,170,9647,7258,9743,5481,6561,1854,6873,5184,7017,7840,1300,4670,3506,3826,9660,8017,8487,3849,7814,4757,8417,1016,3275,2068,9822,2144,4719,4373,9212,1531,1550,9648,3962,2838,3850,9576,4589,2439,7732,9630,4922,8496,5185,4417,692,8803,7863,1520,1900,8129,8172,1321,5960,5921,5905,6759,9956,451,696,7701,2623,6408,3711,2723,6522,6090,294,4949,598,4231,8095,4015,7099,3191,5003,5655,3810,2795,8811,3967,8505,1667,2026,7388,4275,4343,3562,77,9531,4935,2222,3237,4039,7998,3941,9557,5064,2145,5407,293,6953,2107,2976,2654,6863,221,7671,6971,6247,6548,3394,7471,3729,7640,8844,8112,3796,3743,235,6135,9144,2983,9780,6500,1208,2866,4966,4178,1597,4447,7759,8611,547,7918,5758,9049,6049,8750,8810,7251,8006,5281,6309,4188,7900,5736,1106,2748,4225,1137,8850,6043,3848,4359,1159,7560,306,8997,5199,612,2144,5322,2465,2583,7857,6486,4114,9943,1901,1275,488,7272,9650,5846,3370,3471,8663,1928,5378,6291,6275,8835,4312,510,5712,1766,4299,4222,2365,1482,6761,7461,6251,6271,6937,3757,5236,6391,6925,748,7527,3722,3456,8833,348,1056,4697,825,8959,5203,6650,762,1518,9679,5318,6036,4351,1985,7940,8435,5392,8729,1033,936,4189,617,4525,3403,7866,7296,752,2508,2687,9647,2621,583,5851,7256,1496,8228,4446,6964,1659,5499,7490,5489,1509,6933,5846,4134,2986,9109,3368,6619,3500,2429,8959,3495,2599,3529,3275,8184,6572,817,4957,6729,1676,6227,6821,6247,3422,5952,9901,6199,6941,2176,3721,1216,3624,5801,1010,5465,3757,7635,9014,8894,8438,5766,1491,705,3299,9320,7004,6712,2623,4860,3600,1216,2487,7522,9734,2334,123,8381,9169,6954,9181,2056,1919,3817,9254,6730,4104,6766,7234,9281,9718,1474,4920,1151,2510,9857,1891,4740,6445,1724,7147,1779,9534,4628,2352,8052,3536,707,7904,3245,3144,8821,6099,4227,7232,8217,6186,2949,7414,7160,4045,3317,8941,2700,8571,7827,4229,5645,6167,7473,9745,1403,469,4583,9069,1168,8707,7320,6008,4113,5424,6863,2904,1234,2466,1763,4222,6378,7622,5228,4738,814,53,6636,58,1379,7445,5233,4641,973,2904,2971,9041,8629,7119,105,5393,6157,5151,7156,835,2040,8326,3468,4010,5368,4674,9660,8653,2984,2208,5479,157,5657,2456,2899,9974,2965,7244,4695,748,4106,4588,8129,2827,2591,431,2653,3902,4488,6109,9364,2896,5221,9046,9864,6080,8466,4625,8741,8966,1666,1205,5378,2927,1840,8957,7572,2324,1052,6966,8763,2125,3461,5850,3289,9894,4498,7475,5410,8146,2183,1942,3660,7952,1114,9018,4048,6826,2962,5316,8286,4594,5877,3049,5719,8667,2261,2568,8512,9293,139,3417,1943,9412,3756,8597,6981,9306,1741,9616,5408,7381,8644,5165,3953,7430,6656,8464,152,7063,1565,3921,1264,2257,6199,8973,3515,1622,4704,4701,9537,9222,3482,4213,1632,9010,2403,3884,9286,5707,3255,7645,5735,2229,1793,9446,1005,6397,1017,4927,4229,5290,3998,278,4009,2747,4230,5828,9251,9263,5637,951,499,9939,8253,2437,443,8993,1491,370,2120,6500,8371,1257,8434,9444,9433,6658,2963,666,5943,5416,7875,7187,3577,6301,8197,4262,9841,1578,4807,3193,6553,6375,9161,3390,5179,2591,7718,8938,5572,8909,8075,4448,6183,5603,3163,7775,6617,593,2774,6445,8518,5254,4136,5277,2615,426,5935,328,6514,3487,3508,2010};

int b[1000];


EM_PORT_API(void) mainloop(){
	int len=1000;
    for(int i=0;i<100000;i++){
		for(int i1=0;i1<len;i1++){
			b[i1]=a[i1];
		}
        quicksort(b,len);
    }
}

#if !defined(__PWART__)
#include <time.h>
#include <stdio.h>
#endif

#if !defined(__PWART__)
int main(){
	clock_t c=clock();
	mainloop();
	printf("time consumed:%d ms\n",(int)((clock()-c)/(CLOCKS_PER_SEC/1000)));
    return 0;
}
#endif