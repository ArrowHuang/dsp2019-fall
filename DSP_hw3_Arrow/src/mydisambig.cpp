#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>
#include "Ngram.h"

using namespace std;

// 計算bigram分數
double BigramScore(const char *word1,const char *word2, Vocab &vocab, Ngram &lm){
	// 檢測word1是否在Vocab中
    VocabIndex windex1 = vocab.getIndex(word1); 
    if(windex1 == Vocab_None){ 
    	// 出現OOV則用<unk>替代
        windex1 = vocab.getIndex(Vocab_Unknown);
    }
    // 檢測word2是否在Vocab中
    VocabIndex windex2 = vocab.getIndex(word2); 
    if(windex2 == Vocab_None){  
    	// 出現OOV則用<unk>替代
        windex2 = vocab.getIndex(Vocab_Unknown);
    }
    
    VocabIndex context[] = { windex1, Vocab_None };
    return lm.wordProb(windex2, context); // 出現word2|word1的機率
}

// 建立樹結構
struct TreeNode{
    string words;
    string allwords;
    float bigramscore;
    struct TreeNode *prenode;
};

int main(int argc,char *argv[]){	
	
	char seg_file[40] ,zhu2Big5[40] ,lm_model[40], output_file[40];
	if(argc!=5)
	{
		printf("INPUT FORMAT ERROR\n");
		printf("Input Format: ./mydisambig <Segemented file> <ZhuYin-Big5 map> <Language model> <Output file>\n");
		return 0;
	}
	else
	{
		strcpy( seg_file, argv[1] );
		strcpy( zhu2Big5, argv[2] );
		strcpy( lm_model, argv[3] );
		strcpy( output_file, argv[4] );
	}
	
	// 讀取language model
	int ngram_order = 2; //bigram 2-gram
    Vocab voc;
    Ngram lm_file( voc, ngram_order );
    {
        File lmf( lm_model, "r" );
        lm_file.read(lmf);
        lmf.close();
    }

    // cout << BigramScore("視","新",voc,lm_file) << endl;
					
    int num = 0;   										// 儲存每一個空格位置
    string each_big5; 									// 儲存每一個按照空格切開後的big5
    string each_sent;									// 儲存每一個按照空格切開後的sentence
    string line;   										// 儲存每一行
    string ZhuYin; 										// 儲存注音
    string Big5;   										// 儲存中文字
    map< string, vector<string> > Zhuyin_Big5map; 	  	// 創建map檔案儲存ZhuYin-Big5
    vector<string>::iterator itersen;                 	// 創建sentence迭代器
    vector<string>::iterator itermap;				  	// 創建map迭代器讀取前一個注音
    vector<string> answer;								// 創建Vector儲存答案
 
    // 讀取ZhuYin-Big5.map檔案
    ifstream ZhuYin_Big5_file(zhu2Big5);
    if(ZhuYin_Big5_file.is_open()){

    	while ( getline(ZhuYin_Big5_file, line) ) 		// line中不包括每行的換行符
		{ 
			ZhuYin = line.substr(0, line.find(" "));
			Big5 = line.substr(line.find(" ")+1, line.size());
			// cout << ZhuYin << ' ' << Big5 << endl;

    		vector<string> Big5Vector;					// 創建vector儲存每一個Big的檔案
			while( (num = Big5.find(" ")) != string::npos )
			{
				each_big5 = Big5.substr(0,num);
				if( each_big5 != "" ){
					Big5Vector.push_back(each_big5);
				}
				Big5.erase(0,num+1);
			}

			// for(itermap = Big5Vector.begin(); itermap != Big5Vector.end(); ++itermap ){
   	//              cout << (*itermap) << " ";
   //          }
   //          cout << endl;

			Zhuyin_Big5map.insert(pair<string, vector<string>>(ZhuYin,Big5Vector));
		}
		vector<string> temp1;
		temp1.push_back("<s>");
		Zhuyin_Big5map.insert(pair<string, vector<string>>("<s>",temp1));
		vector<string> temp2;
		temp2.push_back("</s>");
		Zhuyin_Big5map.insert(pair<string, vector<string>>("</s>",temp2));

		ZhuYin_Big5_file.close();

    }
    else {
    	cout << "No such file" <<  endl;
    }
 	
    // 讀取切割好後的語料
    ifstream segmented_data(seg_file);
    if(segmented_data.is_open()){

		int times = 0;
    	while ( getline(segmented_data, line) ) 		   // line中不包括每行的換行符
		{ 
			vector<string> SenVector;					   // 創建vector儲存每一個sentence的檔案
			SenVector.push_back("<s>");
			while((num = line.find(" ")) != string::npos )
			{
				each_sent = line.substr(0,num);
				if( each_sent != "" ){
					SenVector.push_back(each_sent);
				} 
				line.erase(0,num+1);
			}
			if(num = line.find("\n")){
				SenVector.push_back("</s>");
			}

			// for(itersen = SenVector.begin(); itersen != SenVector.end(); ++itersen ){
   //              cout << (*itersen) << " ";
   //          }
   //          cout << endl;
			    
    		typedef struct TreeNode TNode;				// 宣告樹狀資料結構TNode
			vector<vector<TNode> > TNodeVec;			// 創建樹狀二維Vector
			int c = 0;                          		// 儲存次數
			float score;								// 儲存分數
			string open = "<s>";						// 儲存第一個
			string AllWord;								// 儲存之前所有的話
			int lengths = SenVector.size();				// 紀錄一共幾個字
			TNode bestnode;								// 儲存最後</s>的最好分數的節點

			// 每一個句子中的字
			for(itersen = SenVector.begin()+1; itersen != SenVector.end(); itersen++)
			{	
				vector<TNode> subTNodeVec;				// 創建樹狀一維Vector

				// 每一個注音
				for(itermap = Zhuyin_Big5map[*(itersen)].begin(); itermap != Zhuyin_Big5map[*(itersen)].end(); itermap++)
				{	
					
					// cout << (*itermap) << endl;
					TNode tnode;							// 創建樹節點
					float maxscore = -99999;				// 儲存最大分數
					float bestscore = -99999;				// 儲存最後</s>的最好的分數
					TNode *maxnode;							// 找到最佳路徑的前一個狀態
					
					// 計算開始<s>與第一個
					if(c==0){
						score = BigramScore(open.c_str(),(*itermap).c_str(),voc,lm_file);
						tnode.bigramscore = score;
						tnode.words = (*itermap);
						tnode.allwords = open+" "+(*itermap);
						tnode.prenode = NULL;
					}
					// 計算第二個之後
					else{
						// 計算前一個狀態
						for(int i=0;i<TNodeVec[(c-1)].size();i++){
							TNode tmpnode = TNodeVec[(c-1)][i];
							score = BigramScore(tmpnode.words.c_str(),(*itermap).c_str(),voc,lm_file);
							float tmp_score = tmpnode.bigramscore + score;
							if(maxscore<tmp_score){
								maxscore = tmp_score;
								maxnode = &TNodeVec[(c-1)][i];
								AllWord = tmpnode.allwords;
							}
								
						}
						tnode.bigramscore = maxscore;
						tnode.words = (*itermap);
						tnode.prenode = maxnode;
						tnode.allwords = AllWord+" "+(*itermap);
						// cout << tnode.allwords << endl;

						if(c==(lengths-2)){
							if(bestscore<maxscore){
								bestscore = maxscore;
								bestnode = tnode;
							}
						}

					}
					subTNodeVec.push_back(tnode);				
				}
				TNodeVec.push_back(subTNodeVec);
				c = c+1;
			}
			answer.push_back(bestnode.allwords);
			// cout << bestnode.allwords << endl;			
		}
		segmented_data.close();
    }
    else {
    	cout << "No such file" << endl; 
    }

    //寫檔案
    ofstream outfile(output_file); 
	for(vector<string>::iterator it = answer.begin(); it != answer.end(); it++)
	{
		// cout << (*it) << endl;
		outfile << (*it) << endl;
	}
	outfile.close();

	return 0;
}