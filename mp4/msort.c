/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struct for passing qsort parameters through pthread_create
struct qsort_struct {
	void *base;
	size_t num;
};

// Struct for passing msort parameters through pthread_create
struct msort_struct {
	int *base1;
	int *base2;
	int count1;
	int count2;
};

// Compare function used by qsort
int compare(const void* p1, const void* p2) {
	int one = *(int *)p1;
	int two = *(int *)p2;
	return one - two;	
}

// Function that calls qsort
void *qsort_thread(void *args) {
	struct qsort_struct *this = (struct qsort_struct *)args;
	qsort(this->base, this->num, sizeof(int), compare);
	fprintf(stderr, "Sorted %d elements.\n", (int)this->num);
	return NULL;
}

// Mergesort function
void *merge(void *args) {
	struct msort_struct *this = (struct msort_struct *)args;
	int *temp = malloc(sizeof(int) * (this->count1 + this->count2));
	int duplicates = 0;
	int *A = this->base1;
	int *B = this->base2;
	int a = 0;
	int b = 0;
	int t = 0;
	// Merge elements of arrays into temp array
	while(a < this->count1 && b < this->count2) {
		if(A[a] <= B[b]) {
			temp[t] = A[a];
			if(A[a] == B[b])
				duplicates ++;
			a++;
		}
		else {
			temp[t] = B[b];
			b++;
		}
		t++;
	}
	while(a < this->count1) {
		temp[t] = A[a];
		a++;
		t++;
	}
	while(b < this->count2) {
		temp[t] = B[b];
		b++;
		t++;
	}
	// Put back into numbers array
	int i;
	for(i = 0; i < this->count1 + this->count2; i++) {
		if(i < this->count1)
			A[i] = temp[i];
		else B[i - this->count1] = temp[i];
	}
	free(temp);
	fprintf(stderr, "Merged %d and %d elements with %d duplicates.\n", this->count1, this->count2, duplicates);
	return NULL;		
}

int main(int argc, char **argv)
{
	// Check parameter is present
	if(argc == 1) {
                fprintf(stderr, "Please enter a segment count.\n");
                return -1;
        }

	// Get numbers from file into an array of int pointers
	int *numbers = malloc(sizeof(int) * 32);
	int size = 32;
	int count = 0;
	char buffer[BUFSIZ];
	while(fgets(buffer, BUFSIZ, stdin) != NULL) {
		if(count == size) {
			numbers = realloc(numbers, sizeof(int) * size * 2);
			size = size * 2;
		}
		numbers[count] = atoi(buffer);
		count++;
	}

	// Split numbers into segments defined by argument list
	int segs = atoi(argv[1]);
	if(segs > count || segs < 1 || (count % segs != 0 && (((count/segs) + 1) * (segs - 1) >= count))) {
		fprintf(stderr, "Please enter a valid segment count.\n");
		return -1;
	}
	int **segment = malloc(sizeof(int *) * segs);
	int per = count / segs;
	if(count % segs != 0)
		per++;
	int i = 0;
	while(i < segs) {
		segment[i] = numbers + (per * i);
		i++; 
	}

	// Launch one thread per segment to sort them using qsort
	pthread_t tid[segs];
	int j = 0;
	struct qsort_struct *arg = malloc(sizeof(struct qsort_struct) * segs);
	while(j < segs) {
		arg[j].base = (void *)segment[j];
		arg[j].num = per;
		if(j == segs - 1 && count % segs != 0) {
			arg[j].num = (size_t)(count % per);
		}
		pthread_create(&(tid[j]), NULL, qsort_thread, &arg[j]); //Passes address of specific arg struct
		j++;
	}
	
	//Wait for threads to finish
	int k;
	for(k = 0; k < segs; k++) {
		pthread_join(tid[k], NULL);
	}

	// Merge sort consecutive segments in rounds of threads
	pthread_t mtid[segs - 1];
	struct msort_struct *marg = malloc(sizeof(struct msort_struct) * (segs - 1)); //segments - 1 merges
	int m = 0;
	int r = 1;
	int n = 0;
	while(m < segs - 1) {
		int s = 0;
		while(s < segs - 1 && s + r < segs) {
			marg[m].base1 = segment[s];
			marg[m].base2 = segment[s + r];
			marg[m].count1 = per * r;
			marg[m].count2 = per * r;
			if(s + r >= segs - r) {
				//If we merge with last segment in the list, make its count equal to the
				//number of elements in the last segment + the number of elements in each segment
				//it has already been merged with
				if(count % segs == 0)
					marg[m].count2 = (per) + (per * (segs - s - r - 1));	
				else marg[m].count2 = (count % per) + (per * (segs - s - r - 1));
			}
			pthread_create(&(mtid[m]), NULL, merge, &marg[m]);
			s = s + (2 * r);
			m++;
		}
		// Wait for round of threads to finish
		int p;
		for(p = n; p < m; p++) {
			pthread_join(mtid[p], NULL);
		}
		n = m;
		r = r * 2;
	}

	// Print list of sorted numbers
	int x = 0;
	while(x < count) {
		printf("%d\n", numbers[x]);
		x++;
	}
	
	// Free up memory and quit
	free(arg);
	free(marg);
	free(segment);
	free(numbers); 
	return 0;
}
