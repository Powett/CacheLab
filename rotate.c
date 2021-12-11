/*
 * Implemented by Nathan PELUSO for INF559 Course
 * rotate.c - Natrix rotation
 *
 * Each rotation function must have a prototype of the form:
 * void rotate(int N, int A[N][N], int B[N][N]);
 *
 * A rotate function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 

#include <stdio.h>
#include "cachelab.h"



void printMatrix(int N, int A[N][N]){
  int row,col;
  FILE* fp;
  fp=fopen("log.txt","a");
  for (row=0;row<N;++row){
    for (col=0;col<N;++col){
      fprintf(fp,"%x ", A[row][col]%255);
    }
    fprintf(fp,"\n");
  }
  fprintf(fp,"*********\n");
  fclose(fp);
}


int is_rotate(int N, int A[N][N], int B[N][N]);
void rotate_col_swapline_t(int N,int A[N][N], int B[N][N]);

/* 
 * rotate_submit - This is the solution rotate function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Rotate submission", as the driver
 *     searches for that string to identify the rotate function to
 *     be graded. 
 */
char rotate_submit_desc[] = "Rotate submission";
void rotate_submit(int N, int A[N][N], int B[N][N])
{
  if (N==32){
    rotate_32_best(32,A,B);
  }else if(N==64){
    //rotate_64_best(64,A,B);
    rotate_64_like_32(64,A,B);
  }else{
    rotate_67_best(67,A,B);
  }
}

/* 
 * You can define additional rotate functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * rotate - A simple baseline rotate function, not optimized for the cache.
 */
char rotate_desc[] = "Simple row-wise scan rotate";
void rotate(int N, int A[N][N], int B[N][N])
{
  int row, col;
  for(row=0; row < N; ++row) {
    for(col=0; col < N; ++col) {
      B[col][N-1-row] = A[row][col];
    }
  }
}

char rotate_swapline_t_desc[] = "Swap lines of A then transpose B";
void rotate_col_swapline_t(int N, int A[N][N], int B[N][N]) //759
{
  int row, col;
  int temp;
  for (row=N-1;row>=0;--row){
    for (col=0;col<N;++col){
      B[row][col]=A[N-1-row][col];
    }
  }
  for (row=0;row<N-1;row++){
    for(col=row+1;col<N;col++){
      temp= B[row][col];
      B[row][col]=B[col][row];
      B[col][row]=temp;
    }
  }
}

int prevGrey(int x){
  // This function is used to get the offset of data to copy into the current (i,j)~2*i+j block in a 4-block matrix
  //  |0 1|  |2 0|   
  //  |2 3|  |3 1|
  // This happens to be the previous number in Grey code, hence the name
  switch (x){
    case 0:
      return 2;
    case 1:
      return 0;
    case 2:
      return 3;
    case 3:
      return 1;
    default:
      return -1;
  }
}

char rotate_32_best_desc[] = "8-Blocking, with lines switching-transposing for better rotation";
void rotate_32_best(int N, int A[32][32], int B[32][32]) //275
{
  int big_block,small_block;
  int i,j,temp;
  int offset_i,offset_j;
  for (big_block=0;big_block<4;big_block++){
    for (small_block=0;small_block<4;small_block++){
      offset_i=(big_block>>1)*16+(small_block>>1)*8;
      offset_j=(big_block%2)*16+(small_block%2)*8;
      for (i=7;i>=0;--i){
        for (j=0;j<8;++j){
          B[i+offset_i][j+offset_j]=A[32-1-i- offset_j][j+offset_i];
        }
      }
      for (i=0;i<7;i++){
        for(j=i+1;j<8;j++){
          temp=B[i+offset_i][j+offset_j];
          B[i+offset_i][j+offset_j]=B[j+offset_i][i+offset_j];
          B[j+offset_i][i+offset_j]=temp;
        }
      }
    }
  }
}

char rotate_64_like_32_desc[] = "8-Blocking, with lines switching-transposing for better rotation";
void rotate_64_like_32(int N, int A[64][64], int B[64][64]) //
{
  int bigger_block,big_block,small_block,smaller_block;
  int i,j,temp;
  int offset_i,offset_j;
  // DEBUG
  // for (i=0;i<64;i++){
  //   for (j=0;j<64;j++){
  //     B[i][j]=0;
  //   }
  // }
  
  for (bigger_block=0;bigger_block<4;bigger_block++){ 
    for (big_block=0;big_block<4;big_block++){
      for (small_block=0;small_block<4;small_block++){   
        for (smaller_block=0;smaller_block<4;smaller_block++){
          // printMatrix(64,B);
          offset_i=(bigger_block>>1)*32+(big_block>>1)*16+(small_block>>1)*8+(smaller_block>>1)*4;
          offset_j=(bigger_block%2)*32+(big_block%2)*16+(small_block%2)*8+(smaller_block%2)*4;
          for (i=3;i>=0;--i){
            for (j=0;j<4;++j){
              B[i+offset_i][j+offset_j]=A[64-1-i - offset_j][j+offset_i];
            }
          }
          for (i=0;i<3;i++){
            for(j=i+1;j<4;j++){
              temp=B[i+offset_i][j+offset_j];
              B[i+offset_i][j+offset_j]=B[j+offset_i][i+offset_j];
              B[j+offset_i][i+offset_j]=temp;
            }
          }
        }
      }
    }
  }
}

char rotate_64_best_desc[] = "Copy rectangle blocks from A and put in B. Best Height*Width (8*4) found experimentally";
void rotate_64_best(int N, int A[N][N], int B[N][N])
{
  int v1,v2,v3,v4;
  int i,j,k;
  for (i=0;i<N;i+=8){
    for (j=0;j<N;j+=4){
      for (k=(i+7 > N-1 ? N-1 : i+7);k>=i;--k){
        v1=A[k][j];
        v2=A[k][j+1];
        v3=A[k][j+2];
        v4=A[k][j+3];

        B[j][N-1-k]=v1;
        B[j+1][N-1-k]=v2;
        B[j+2][N-1-k]=v3;
        B[j+3][N-1-k]=v4;
      }
    }
  }
}

void rotate_64_best_v0(int N, int A[N][N], int B[N][N])
{
  int v1,v2,v3,v4;
  int i,j,k;
  for (i=0;i<N;i+=8){
    for (j=0;j<N;j+=4){
      for (k=(i+7 > N-1 ? N-1 : i+7);k>=i;--k){
        v1=A[k][j];
        v2=A[k][j+1];
        v3=A[k][j+2];
        v4=A[k][j+3];

        B[j][N-1-k]=v1;
        B[j+1][N-1-k]=v2;
        B[j+2][N-1-k]=v3;
        B[j+3][N-1-k]=v4;
      }
    }
  }
}


char rotate_67_border_crown_core_desc[] = "Manually filling border (last row & left column of B), rotating crown(1-width square around the 64x64 core), then handling the core like before";
void rotate_67_border_crown_core(int N, int A[67][67], int B[67][67]) //275
{
  int i,j,temp;
  int bigger_block,big_block,small_block;
  int offset_i,offset_j;

  for (i=0;i<67;i++){
    B[i][0]=A[66][i];
  }
  for (j=0;j<67;j++){
    B[66][j]=A[67-1-j][66];
  }

  for (j=0;j<66;j++){
    B[0][j+1]=A[65-j][0];
  }
  for (j=0;j<66;j++){
    B[65][j+1]=A[65-j][65];
  }
  for (i=1;i<65;i++){
    B[i][0+1]=A[65][i];
    B[i][65+1]=A[0][i];
  }
  
  for (bigger_block=0;bigger_block<4;bigger_block++){
    for (big_block=0;big_block<4;big_block++){
      for (small_block=0;small_block<4;small_block++){
        offset_i=(bigger_block>>1)*32+(big_block>>1)*16+(small_block>>1)*8;
        offset_j=(bigger_block%2)*32+(big_block%2)*16+(small_block%2)*8;
        for (i=7;i>=0;--i){
          for (j=0;j<8;++j){
            B[i+offset_i + 1 ][j+offset_j + 2 ]=A[64-1-offset_j-i + 1 ][j+offset_i + 1 ];
          }
        }
        for (i=0;i<7;i++){
          for(j=i+1;j<8;j++){
            temp=B[i+offset_i +1][j+offset_j +2];
            B[i+offset_i +1 ][j+offset_j +2]=B[j+offset_i +1 ][i+offset_j +2 ];
            B[j+offset_i +1 ][i+offset_j +2]=temp;
          }
        }
      }
    }
  }
}

char rotate_67_3border_core_desc[] = "Rotating 3-width border blocks, then handling the core like before";
void rotate_67_3border_core(int N, int A[67][67], int B[67][67])
{
  int i,j,temp;
  int bigger_block,big_block,small_block;
  int offset_i,offset_j;

  // first columns of B
  for (small_block=0;small_block<8;small_block++){
    for (i=7;i>=0;--i){
      for (j=0;j<3;j++){        
        B[i+small_block*8][j]=A[64+3-1-j][i+small_block*8];
      }
    }
  }
  // last rows of B
  for (small_block=8;small_block>=0;--small_block){
    for (i=2;i>=0;i--){
      for (j=0;j<8;j++){
        B[64+i][small_block*8 + j ]=A[67-1-j-8*small_block][64+i];
      }
    }
  }
  // last 3x3 block
  for (i=0;i<3;i++){
    for (j=0;j<3;j++){
      B[i+64][j]=A[3-1-j+64][i+64];
    }
  }

  for (bigger_block=0;bigger_block<4;bigger_block++){
    for (big_block=0;big_block<4;big_block++){
      for (small_block=0;small_block<4;small_block++){
        offset_i=(bigger_block>>1)*32+(big_block>>1)*16+(small_block>>1)*8;
        offset_j=(bigger_block%2)*32+(big_block%2)*16+(small_block%2)*8;
        for (i=7;i>=0;--i){
          for (j=0;j<8;++j){
            B[i+offset_i  ][j+offset_j + 3 ]=A[64-1-offset_j-i  ][j+offset_i  ];
          }
        }
        for (i=0;i<7;i++){
          for(j=i+1;j<8;j++){
            temp=B[i+offset_i ][j+offset_j +3];
            B[i+offset_i ][j+offset_j +3]=B[j+offset_i  ][i+offset_j +3 ];
            B[j+offset_i  ][i+offset_j +3]=temp;
          }
        }
      }
    }
  }
}

char rotate_67_best_desc[] = "Copy rectangle blocks from A and put in B. Best Height*Width (17*4) found experimentally";
void rotate_67_best(int N, int A[N][N], int B[N][N])
{
  int v1,v2,v3,v4;
  int i,j,k;
  for (i=0;i<N;i+=17){
    for (j=0;j<N;j+=4){
      for (k=(i+16 > N-1 ? N-1 : i+16);k>=i;--k){
        v1=A[k][j];
        v2=A[k][j+1];
        v3=A[k][j+2];
        v4=A[k][j+3];

        B[j][N-1-k]=v1;
        B[j+1][N-1-k]=v2;
        B[j+2][N-1-k]=v3;
        B[j+3][N-1-k]=v4;
      }
    }
  }
}


/*
 * registerFunctions - This function registers your rotate
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     rotate strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerRotateFunction(rotate_submit, rotate_submit_desc); 
    //registerRotateFunction(rotate_64_like_32,rotate_64_like_32_desc);
    /* Register any additional rotate functions */
    //registerRotateFunction(rotate, rotate_desc); 
    //registerRotateFunction(rotate_67_3border_core,rotate_67_3border_core_desc);
    //registerRotateFunction(rotate_67_border_crown_core,rotate_67_border_crown_core_desc);

}

/* 
 * is_rotate - This helper function checks if B is the rotate of
 *     A. You can check the correctness of your rotate by calling
 *     it before returning from the rotate function.
 */
int is_rotate(int N, int A[N][N], int B[N][N])
{
  int row, col;
  
  for(row=0; row < N; ++row) {
    for(col=0; col < N; ++col) {
      if (B[col][N-1-row] != A[row][col])
	return 0;
    }
  }
  return 1;
}