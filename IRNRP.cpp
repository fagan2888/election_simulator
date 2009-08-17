#include "Voter.h"
#include "IRNRP.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void IRNRP::init( const char** envp ) {
	const char* cur = *envp;
	while (cur != NULL) {
		if (0 == strncmp(cur, "seats=", 6)) {
			seats = strtol(cur + 6, NULL, 10);
		}
		envp++;
		cur = *envp;
	}
}

/*
 If a legallistic definiton specified 'not more than one part in one thousand'
 or similar the implementation might need to be redone in integer millivotes
 or some specific integer fraction thereof.
 */
static const double overtallyEpsilon = 1.001;

/*
 Sum normalized ratings
 Calculate quota
 Deweight over-quota choices
 If over-quota sum less than epsilon and insufficient winners, disqualify lowest.
 */
void IRNRP::runElection( int* winnerR, const VoterArray& they ) {
    int i;
    int numc = they.numc;
    int numv = they.numv;
    double* tally = new double[numc];
	double* cweight = new double[numc];
	bool* active = new bool[numc];
	bool* winning = new bool[numc];
	int roundcounter = 1;
	int numactive = numc;
	
	for (int c = 0; c < numc; ++c) {
		active[c] = true;
		winning[c] = false;
		cweight[c] = 1.0;
	}
    
	while (true) {
		// round init
		for (int c = 0; c < numc; ++c) {
			tally[c] = 0.0;
		}
		
		// count normalized votes for each candidate
		for ( i = 0; i < numv; i++ ) {
			double vsum = 0.0;
			double minp = 0.0;
			for (int c = 0; c < numc; ++c) {
				if (active[c]) {
					double p = they[i].getPref(c);
					if (p < minp) {
						minp = p;
					}
					double t = p * cweight[c];
					vsum += t * t;
				}
			}
			if (minp < 0.0) {
				vsum = 0.0;
				for (int c = 0; c < numc; ++c) {
					if (active[c]) {
						double p = they[i].getPref(c) - minp;
						double t = p * cweight[c];
						vsum += t * t;
					}
				}
			}
			if (vsum <= 0.0) {
				continue;
			}
			vsum = 1.0 / sqrt(vsum);
			for (int c = 0; c < numc; ++c) {
				if (active[c]) {
					double p = they[i].getPref(c) - minp;
					assert(p >= 0.0);
					tally[c] += p * cweight[c] * vsum;
				}
			}
		}
		double totalvote = 0.0;
		for (int c = 0; c < numc; ++c) {
			if (active[c]) {
				assert(tally[c] > 0.0);
				totalvote += tally[c];
			}
		}
		double quota = totalvote / (seats + 1.0);
		double quotaPlusEpsilon = quota * overtallyEpsilon;
		//fprintf(stderr, "%d total vote=%f, quota for %d seats is %f\n", roundcounter, totalvote, seats, quota);
		int numwinners = 0;
		bool cweightAdjusted = false;
		for (int c = 0; c < numc; ++c) {
			if (tally[c] > quota) {
				winning[c] = true;
				numwinners++;
				if (tally[c] > quotaPlusEpsilon) {
					//fprintf(stderr, "%d tally[%d]=%f > %f. old cweight=%f\n", roundcounter, c, tally[c], quotaPlusEpsilon, cweight[c]);
					cweight[c] *= (quota / tally[c]);
					cweightAdjusted = true;
					//fprintf(stderr, "%d tally[%d]=%f > %f. new cweight=%f\n", roundcounter, c, tally[c], quotaPlusEpsilon, cweight[c]);
				}
			} else if (winning[c]) {
				numwinners++;
				fprintf(
					stderr, "%d warning, winning choice(%d) has fallen below quota (%f < %f)\n",
					roundcounter, c, tally[c], quota);
			}
		}
#if 0
		for (int c = 0; c < numc; ++c) {
			fprintf(stderr, "%d tally[%d] = %f, cweight=>%f\n", roundcounter, c, tally[c], cweight[c]);
		}
#endif
		if (numwinners == seats) {
			break;
		}
		if (cweightAdjusted) {
			// re-run with new weights
		} else {
			// disqualify a loser.
			double mint = 9999999.0;
			int mini = -1;
			for (int c = 0; c < numc; ++c) {
				if (active[c]) {
					if ((mini == -1) || (tally[c] < mint)) {
						mint = tally[c];
						mini = c;
					}
				}
			}
			assert(mini != -1);
			active[mini] = false;
			numactive--;
			if (numactive == seats) {
				// that which remains, wins.
				for (int c = 0; c < numc; ++c) {
					winning[c] = active[c];
				}
				break;
			}
			// reset cweights
			for (int c = 0; c < numc; ++c) {
				cweight[c] = 1.0;
			}
		}
		roundcounter++;
	}
	//fprintf(stderr, "finished IRNRP after %d rounds\n", roundcounter);
    // return winners
	int winneri = 0;
	for (int c = 0; c < numc; ++c) {
		if (winning[c]) {
			winnerR[winneri] = c;
			winneri++;
		}
	}
	delete [] winning;
	delete [] active;
	delete [] cweight;
    delete [] tally;
}

VotingSystem* newIRNRP( const char* n ) {
	return new IRNRP();
}
VSFactory* IRNRP_f = new VSFactory( newIRNRP, "IRNRP" );

IRNRP::~IRNRP() {
}