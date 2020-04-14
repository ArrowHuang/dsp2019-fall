#include "hmm.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SEQ_ROW 2500  //max row num of test_seq
#define MAX_SEQ_COL 50    //max column num of test_seq
#define MAX_STATE	10    //max state num of initial model
#define TOTAL_STATE 6     //total state num
#define FILE_NUM 20

char test_data[MAX_SEQ_ROW][MAX_SEQ_COL]; 			//store testing sequence
char model_answer[MAX_SEQ_ROW][FILE_NUM];
double model_prob[MAX_SEQ_ROW];

/* Read Testing Sequence */
void read_test_seq(char *seq_path){
	FILE *finput;
	char buf[MAX_SEQ_COL + 1]; 
	finput=fopen(seq_path,"r"); 
	if(finput==NULL) 
	{
		printf("Fail To Open File %s!\n",seq_path);
  		return;
  	}
  	for (int i = 0; i < MAX_SEQ_ROW; i++)
    {
        fscanf(finput, "%s", buf);
        for (int j = 0; j < MAX_SEQ_COL; j++)
        {
            test_data[i][j] = buf[j];
        }
    }
    fclose(finput);
}

/* Viterbi Algorithm */
void viterbi_algo(HMM *hmm, char *test_seq, int index){
	int model_num = 0;
	double max_model = -1;	
	char file_name[FILE_NUM];

	for(int num=0;num<5;num++){

		double delta[MAX_SEQ_ROW][TOTAL_STATE] = {0};  //variable of viterbi algorithm
		double *pi = hmm[num].initial;       			   //initial state
		double (*a)[MAX_STATE] = hmm[num].transition;      //state transition 
		double (*b)[MAX_STATE] = hmm[num].observation;     //state observation
		int id; double max; double max_num;

		/* Initialization */
		for(int i=0;i<TOTAL_STATE;i++){
			delta[0][i] = pi[i] * b[test_seq[0]-'A'][i];
		}

		/* Recursion */
		for(int t=1;t<MAX_SEQ_COL;t++){
			id = test_seq[t]-'A';
			for(int j=0;j<TOTAL_STATE;j++){
				max = -1; 
				for(int i=0;i<TOTAL_STATE;i++){
					double sum = delta[t-1][i] * a[i][j];
					if(sum>max){
						max = sum;
						// printf("%.10e\n",max );
					}
				}
				delta[t][j] = max * b[id][j];
			}	
		}

		/* Termination */
		max_num = -1;
		for(int i=0;i<TOTAL_STATE;i++){
			if(delta[MAX_SEQ_COL-1][i]>max_num){
				max_num = delta[MAX_SEQ_COL-1][i];
			}
		}
		if (max_num > max_model){
			max_model = max_num;
			model_num = num;
			// printf("%.10e\n",max_model );
		}
	}
	snprintf(file_name, 13, "model_0%d.txt", model_num + 1);
	// printf("%s\n",file_name );
	strncpy(model_answer[index], file_name, 13);
	model_prob[index] = max_model;
}

/* Writer Answer To Text */
void write_answer(char *output_result_path){
	FILE *output;
	output = fopen(output_result_path, "w");
	for (int i = 0; i < MAX_SEQ_ROW; i++)
	{
		fprintf(output, "%s %.10e\n", model_answer[i], model_prob[i]);
	}
}

/* Check Accuracy */
void check_acc(char *right_path){
	double right_num = 0;
	FILE *finput;
	char buf[MAX_SEQ_COL + 1]; 
	finput=fopen(right_path,"r"); 
  	for (int i = 0; i < MAX_SEQ_ROW; i++)
    {
        fscanf(finput, "%s", buf);
        if(strcmp(model_answer[i],buf)==0){
        	right_num += 1;
        }
    }
    printf("%f\n",(right_num/MAX_SEQ_ROW));
    fclose(finput);
}

int main(int argc,char *argv[]){
	int index;
	char models_list_path[40] ,seq_path[40] ,output_result_path[40];

	if(argc!=4)
	{
		printf("INPUT FORMAT ERROR\n");
		printf("Input Format: ./test <models_list_path> <seq_path> <output_result_path>\n");
		return 0;
	}
	else
	{
		strcpy( models_list_path, argv[1] );
		strcpy( seq_path, argv[2] );
		strcpy( output_result_path, argv[3] );
	}

	/* Read Testing Sequence */
	read_test_seq(seq_path);

	/* Load HMM Model */
	HMM hmm[5];
	load_models(models_list_path, hmm, 5);
	
	/* Start Testing */
	for(int i=0;i<MAX_SEQ_ROW;i++){
		/* Viterbi Algorithm */
		// printf("%d\n", i);
		index = i;
		viterbi_algo(hmm,test_data[i],index);
	}

	/* Writer Answer To Text */
	write_answer(output_result_path);

	/* Check Accuracy */
	// char right_path[20];
	// strcpy( right_path, "data/test_lbl.txt" );
	// check_acc(right_path);

	return 0;
}