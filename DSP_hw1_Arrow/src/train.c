#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hmm.h"
#define MAX_SEQ_ROW 10000 //max row num of train_seq
#define MAX_SEQ_COL 50    //max column num of train_seq
#define MAX_STATE	10    //max state num of initial model
#define TOTAL_STATE 6     //total state num
#define MAX_OBSERV	26    //max observation num of initial model

double alpha[MAX_SEQ_COL][MAX_STATE]; 	   				//forward algorithm variable
double beta[MAX_SEQ_COL][MAX_STATE];	   				//backward algorithm variable
double gama[TOTAL_STATE][MAX_SEQ_COL];	   				//gamma calculated by alpha and beta
double epliso[MAX_SEQ_COL-1][TOTAL_STATE][TOTAL_STATE]; //epliso calculated by alpha and beta
double gama_k[TOTAL_STATE][TOTAL_STATE];
// double epliso_sum[TOTAL_STATE][TOTAL_STATE];			//epliso summarizaton
// double gama_sum[TOTAL_STATE][MAX_SEQ_COL];			//gamma summarizaton
char train_data[MAX_SEQ_ROW][MAX_SEQ_COL]; 				//store training sequence

/* Read Training Sequence */
void read_tran_seq(char *seq_path){
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
            train_data[i][j] = buf[j];
        }
    }
    fclose(finput);
}

/* Output Model After Training */
void output_model(HMM *hmm, char *output_path){
	FILE *output;
	output = fopen(output_path, "w");
    dumpHMM(output, hmm);
}

/* Forward Algorithm */
void forward_algo(HMM *hmm, char *train_seq){
	double *pi = hmm->initial;       //initial state
	double (*a)[MAX_STATE] = hmm->transition;  //state transition 
	double (*b)[MAX_STATE] = hmm->observation; //state observation
	int id;
	
	for(int t=0;t<MAX_SEQ_COL;t++){
		id = train_seq[t]-'A';
		for(int j=0;j<TOTAL_STATE;j++){
			if(t==0){
				alpha[t][j] = pi[j] * b[id][j];     //Initialization
			}
			else{
				double sum = 0;
				for(int i=0;i<TOTAL_STATE;i++){
					sum += alpha[t-1][i] * a[i][j]; //Summation
				}
				alpha[t][j] = sum * b[id][j];       //Induction
			}
		}
	}
}

/* Backward Algorithm */
void backward_algo(HMM *hmm, char *train_seq){
	double (*a)[MAX_STATE] = hmm->transition;  //state transition 
	double (*b)[MAX_STATE] = hmm->observation; //state observation
	int id;
	
	for(int i=0;i<TOTAL_STATE;i++)
		beta[MAX_SEQ_COL-1][i] = 1.0;

	for(int t=MAX_SEQ_COL-2;t>=0;t--){
		id = train_seq[t+1]-'A';
		for(int i=0;i<TOTAL_STATE;i++){
			// if(t==(MAX_SEQ_COL-1)){
			// 	beta[t][i] = 1.0;   //Initialization
			// }
			// else{
				beta[t][i] = 0.0;
				for(int j=0;j<TOTAL_STATE;j++){
					beta[t][i] += (a[i][j] * b[id][j] * beta[t+1][j]); //Induction
				// }
			}
		}
	}
}

/* Claculate Gamma From Alpha And Beta */
void cal_gamma(char *train_seq, double (*gama_k)[TOTAL_STATE]){
	double sum; 
	int id;
	
	for(int t=0;t<MAX_SEQ_COL;t++){
		sum = 0.0;
		id = train_seq[t]-'A';
		
		for(int i=0;i<TOTAL_STATE;i++){
			sum += alpha[t][i] * beta[t][i];
		}
		// printf("%e ",sum);
		for(int i=0;i<TOTAL_STATE;i++){
			gama[i][t] = (alpha[t][i] * beta[t][i])/sum;
			gama_k[id][i] += gama[i][t];
		}
		
	}
	
}

/* Claculate Epliso From Alpha And Beta */
void cal_epliso(HMM *hmm, char *train_seq){
	double (*a)[MAX_STATE] = hmm->transition;  //state transition 
	double (*b)[MAX_STATE] = hmm->observation; //state observation
	double sum; int id;

	for(int t=0;t<MAX_SEQ_COL-1;t++){
		sum = 0;
		id = train_seq[t+1]-'A';
		for(int i=0;i<TOTAL_STATE;i++){
			for(int j=0;j<TOTAL_STATE;j++){
				sum += alpha[t][i] * a[i][j] * b[id][j] * beta[t+1][j];
			}
		}
		for(int i=0;i<TOTAL_STATE;i++){
			for(int j=0;j<TOTAL_STATE;j++){
				epliso[t][i][j] = (alpha[t][i] * a[i][j] * b[id][j] * beta[t+1][j])/sum;
			}
		}

	}
}

/* Update HMM Model */
void update_HMM(HMM *hmm, char *output_path, double (*epliso_sum)[TOTAL_STATE], double (*gama_sum)[MAX_SEQ_COL],double (*gama_k)[TOTAL_STATE]){
	
	double gama_total[TOTAL_STATE] = {0};

	// Update initial state probability pi
	for(int i = 0; i < TOTAL_STATE; i++){
		hmm->initial[i] = gama_sum[i][0] / MAX_SEQ_ROW;
    	// printf("%f ",gama_sum[i][0] / MAX_SEQ_ROW);
    }
  //   printf("\n");
 	// printf("\n");

 	// Total Gamma for state transaction
    for(int t = 0; t < MAX_SEQ_COL-1; t++){ //改not sure 
        for (int i = 0; i < TOTAL_STATE; i++){
            gama_total[i] += gama_sum[i][t];
        }
    } 

	// Update state transition probability
    for(int i = 0; i < TOTAL_STATE; i++){
    	for (int j = 0; j < TOTAL_STATE; j++){
            hmm->transition[i][j] = epliso_sum[i][j] / gama_total[i];
            // printf("%f ",epliso_sum[i][j]/ gama_total[i]);
        }
        // printf("\n");
    }
    // printf("\n");

    // Total Gamma for observation
    for (int i = 0; i < TOTAL_STATE; i++){
        gama_total[i] += gama_sum[i][MAX_SEQ_COL-1];
    }
     

	// Update state observation probability
    for(int i=0;i<TOTAL_STATE;i++){
    	for(int k=0;k<TOTAL_STATE;k++){
    		hmm ->observation[k][i] = gama_k[k][i] / gama_total[i];
    		// printf("%f ",gama_k[k][i]/ gama_total[i]);
    	}
    	// printf("\n");
    }
    // printf("\n");

    /* Output HMM Model */
	output_model( hmm,  output_path);
}

/* Main Function */
int main(int argc,char *argv[]){
	
	int iteration;
	char model_init_path[40] ,seq_path[40] ,output_model_path[40];

	if(argc!=5)
	{
		printf("INPUT FORMAT ERROR\n");
		printf("Input Format: ./train <iter> <model_init_path> <seq_path> <output_model_path>\n");
		return 0;
	}
	else
	{
		iteration = atoi(argv[1]);
		strcpy( model_init_path, argv[2] );
		strcpy( seq_path, argv[3] );
		strcpy( output_model_path, argv[4] );
	}

	/* Read Training Sequence */
	read_tran_seq(seq_path);

	/* Load Initial HMM Model */
	HMM hmm_initial;
	loadHMM( &hmm_initial,  model_init_path);
	
			
	/* Start Training */
	for (int itera = 0; itera < iteration; itera++){
		double epliso_sum[TOTAL_STATE][TOTAL_STATE] = {0.0};
		double gama_sum[TOTAL_STATE][MAX_SEQ_COL] = {0.0};   		   
		
		for(int i=0;i<TOTAL_STATE;i++){
			for(int j=0;j<TOTAL_STATE;j++){
				gama_k[i][j] = 0;
			}
		}

		// for(int i=0;i<MAX_STATE;i++)
		// 	for(int j=0;j<MAX_SEQ_COL;j++)
		// 		gama[i][j]=0.0;
		for (int r = 0; r < MAX_SEQ_ROW; r++){ //MAX_SEQ_ROW

			/* Forward Algorithm to get alpha */
			forward_algo(&hmm_initial,train_data[r]);

			/* Backward Algorithm to get beta */
			backward_algo(&hmm_initial,train_data[r]);

			/* Claculate Gamma From Alpha And Beta */
			cal_gamma(train_data[r],gama_k);
			
			/* Claculate Epliso From Alpha And Beta */
			cal_epliso(&hmm_initial,train_data[r]);

			/* Claculate Gamma Summarization */
			for(int i=0;i<TOTAL_STATE;i++){
				for(int t=0;t<MAX_SEQ_COL;t++){
					gama_sum[i][t] += gama[i][t];
					if(t!=MAX_SEQ_COL-1){
						for(int j=0;j<TOTAL_STATE;j++){
							epliso_sum[i][j] += epliso[t][i][j];
						}
					}
				}
			}

		}
		// double alpha[MAX_SEQ_COL][MAX_STATE]; 	   				//forward algorithm variable
		// double beta[MAX_SEQ_COL][MAX_STATE];	   				//backward algorithm variable
		// double gama[TOTAL_STATE][MAX_SEQ_COL];	   				//gamma calculated by alpha and beta
		// double epliso[MAX_SEQ_COL-1][TOTAL_STATE][TOTAL_STATE]; //epliso calculated by alpha and beta
		// double gama_k[TOTAL_STATE][TOTAL_STATE];
		// for(int id=0;id<MAX_SEQ_COL;id++){
		// 		for(int i=0;i<6;i++){
		// 			printf("%lf ",gama_sum[i][id]);
		// 		}
		// 		printf("\n");
		// 	}
		/* Update HMM Model */
		update_HMM(&hmm_initial,output_model_path,epliso_sum,gama_sum,gama_k);
	
	}
	
	return 0;
}
