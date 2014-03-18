// Ditu Alexandru Mihai 333CA
// Tema 4 APD

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <time.h>
#include "mpi.h"

using namespace std;

#define CAPACITY 50
#define INF 999
#define BUF_SIZE 1000

int readNeighData(int rank, string filename, int *neigh) {
	fstream inFile;
	inFile.open(filename.c_str(), fstream::in);
	string line;
	int count = 0;

	while (getline(inFile, line)) {
		// cout << line << endl;
		istringstream iss(line);
		string no;
		iss >> no;
		int id = atoi(no.c_str());
		if (id == rank) {
			iss >> no; //sar peste -
			while (iss >> no) {
				int nr;
				nr = atoi(no.c_str());
				neigh[count] = nr;
				count++;
			}
		}
	}
	return count;
}

void recvTopFromNeigh(int rank, int size, int *neigh, int top[][CAPACITY], int eco_count,
						int mat_size, vector<int> &banned_neigh) {

	MPI_Status status;
	int new_top[CAPACITY][CAPACITY];
	int val;
	int source;

	while (eco_count > 0) {
		MPI_Recv(&val, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
		
		// daca e o cerere trimit ecou nul
		if (val == 1) {
			val = 3;
			banned_neigh.push_back(status.MPI_SOURCE);
			MPI_Send(&val, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);	
		} else {
			// daca e un ecou il primesc si interpretez
			if (val == 2) {
				source = status.MPI_SOURCE;
				MPI_Recv(new_top, mat_size, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
				for (int i = 0; i < size; i++) {
					for (int j = 0; j < size; j++) {
						top[i][j] = top[i][j] | new_top[i][j];
					}
				}
				eco_count--;
			} else {
				// val == 3, ecou nul, nu fac nimic cu el
				eco_count--;
			}
		}
	}
}

// masterul trimite tuturor topologia noua, asa ca el trimite primul
void masterSendTop (int rank, int size, int *neigh, int top[][CAPACITY],
						int mat_size, int no_of_neigh) {


	if (rank == 0) {
		for (int i = 0; i < no_of_neigh; i++) {
			MPI_Send(top, mat_size, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
		}
	}
}

// toate celelalte procese primesc topologia si o trimit mai departe tuturor,
// mai putin celui de la care au primit si mai putin celor "ban-ati" - explicatii
// suplimentare in Readme
void othersSendTop (int rank, int size, int *neigh, int top[][CAPACITY],
						int mat_size, int no_of_neigh, vector<int> banned_neigh) {

	int new_top[CAPACITY][CAPACITY];
	int parent;
	MPI_Status status;

	MPI_Recv(new_top, mat_size, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
	parent = status.MPI_SOURCE;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			top[i][j] = new_top[i][j];
		}
	}

	for (int i = 0; i < no_of_neigh; i++) {
		if (neigh[i] != parent) {
			int skip = 0;
			for (unsigned j = 0; j < banned_neigh.size(); j++) {
				if (banned_neigh[j] == neigh[i]) {
					skip = 1;
					break;
				}
			}
			if (skip == 0) {
				MPI_Send(new_top, mat_size, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
			}
		}		
	}
}

// functie ce calculeaza tabela de rutare
void FloydWarshall (int top[][CAPACITY], int size, int v[][CAPACITY]) {

	int i, j, k;
	int m[CAPACITY][CAPACITY];

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			if (top[i][j] == 0) {
				if (i == j) {
					m[i][j] = 0;
				} else {
					m[i][j] = INF;
				}
			} else {
				m[i][j] = 1;
			}

			if (m[i][j] < INF) {
				v[i][j] = j;
			}
		}
	}

	for (k = 0; k < size; k++) {
		for (i = 0; i < size; i++) {
			for (j = 0; j < size; j++) {
				if (m[i][j] > m[i][k] + m[k][j]) {
					m[i][j] = m[i][k] + m[k][j];
					v[i][j] = v[i][k];
				}
			}
		}
	}

}

// functie ce scrie in fisier vectorul de rutare
void writeRoutingVec(int v[], int size, int rank, char filename[]) {

	fstream fout;
	fout.open(filename, ios::out);

	fout << rank << endl;
	for (int i = 0; i < size; i++) {
		fout << i << " " << v[i] << endl;
	}

	fout.close();
}

// functie ce afiseaza vectorul de rutare
void showRoutingVec(int v[], int size, int rank) {
    cout << rank << endl;
    for (int i = 0; i < size; i++) {
        cout << i << " " << v[i] << endl;
    }
}

void masterWriteRoutingVec(int v[], int size, int rank, char filename[],
							int top[][CAPACITY]) {

	fstream fout;
	fout.open(filename, ios::out);

	fout << rank << endl;
	for (int i = 0; i < size; i++) {
		fout << i << " " << v[i] << endl;
	}

	fout << endl << "Top:" << endl;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			fout << top[i][j] << " ";
		}
		fout << endl;
	}

	fout.close();
}

int readMessagesFile(int rank, string filename, vector<string> &messages,
					vector<int> &destination) {
	fstream inFile;
	inFile.open(filename.c_str(), fstream::in);
	string line;
	int count = 0;

	getline(inFile, line);
	int no_of_lines = atoi(line.c_str());

	for (int i = 0; i < no_of_lines; i++) {
		getline(inFile, line);

		istringstream iss(line);
		string source, dest, word;
		string message;
		iss >> source;
		iss >> dest;

		
		if (atoi(source.c_str()) == rank) {
			if (dest.compare("B") == 0) {
				destination.push_back(-1);
			} else {
				destination.push_back(atoi(dest.c_str()));
			}

			while (iss >> word) {
				message += word;
				message += " ";
			}
			messages.push_back(message);
			count ++;
		}
	}

	

	inFile.close();

	return count;
}

int findLeader(int rank, int no_of_neigh, int *neigh, vector<int> banned_neigh) {
	    // Exercitiul 3 :  stabilirea liderului
    int no_of_kids;
    int leader_rank = rank;
    vector<int> recv_from;
    int val, parent;
    MPI_Status status;

    no_of_kids = no_of_neigh - banned_neigh.size() - 1;

    // primesc de la copii rank-ul minim
    for (int i = 0; i < no_of_kids; i++) {
    	MPI_Recv(&val, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
    	if (val < leader_rank) {
    		leader_rank = val;
    	}
    	recv_from.push_back(status.MPI_SOURCE);
    }

    // aflu parintele (cel de la care nu am primit nimic)
    parent = -1;
    for (int i = 0; i < no_of_neigh; i++) {
    	int found = 0;
    	for (int j = 0; j < no_of_kids; j++) {
    		if (neigh[i] == recv_from[j]) {
    			found = 1;
    			break;
    		}
    	}

    	if (found == 0) {
    		// verific sa nu fie banned
    		for (unsigned j = 0; j < banned_neigh.size(); j++) {
    			if (neigh[i] == banned_neigh[j]) {
    				found = 1;
    				break;
    			}
    		}
    	}

    	if (found == 0) {
    		parent = neigh[i];
    		//cout << rank << ": parent: " << parent << endl;
    		break;
    	}
    }


    // trimit la parinte rank-ul minim
    MPI_Send(&leader_rank, 1, MPI_INT, parent, 2, MPI_COMM_WORLD);

    // primesc de la parinte rank-ul minim
    // daca e egal cu rank-ul meu, at eu sunt leader
    MPI_Recv(&val, 1, MPI_INT, parent, 2, MPI_COMM_WORLD, &status);

    if (val < leader_rank) {
    	leader_rank = val;
    }

    // trimit la copii rank-ul minim
    for (unsigned i = 0; i < recv_from.size(); i++) {
    	MPI_Send(&leader_rank, 1, MPI_INT, recv_from[i], 2, MPI_COMM_WORLD);
    }

    return leader_rank;
}


int findDeputy(int rank, int no_of_neigh, int *neigh, vector<int> banned_neigh, int leader) {
	    // Exercitiul 3 :  stabilirea liderului
    int no_of_kids;
    int deputy = rank;

    if (deputy == leader) {
    	deputy = INF;
    }

    vector<int> recv_from;
    int val, parent;
    MPI_Status status;

    no_of_kids = no_of_neigh - banned_neigh.size() - 1;

    // primesc de la copii rank-ul minim
    for (int i = 0; i < no_of_kids; i++) {
    	MPI_Recv(&val, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
    	if (val < deputy && val != leader) {
    		deputy = val;
    	}
    	recv_from.push_back(status.MPI_SOURCE);
    }

    // aflu parintele (cel de la care nu am primit nimic)
    parent = -1;
    for (int i = 0; i < no_of_neigh; i++) {
    	int found = 0;
    	for (int j = 0; j < no_of_kids; j++) {
    		if (neigh[i] == recv_from[j]) {
    			found = 1;
    			break;
    		}
    	}

    	if (found == 0) {
    		// verific sa nu fie banned
    		for (unsigned j = 0; j < banned_neigh.size(); j++) {
    			if (neigh[i] == banned_neigh[j]) {
    				found = 1;
    				break;
    			}
    		}
    	}

    	if (found == 0) {
    		parent = neigh[i];
    		//cout << rank << ": parent: " << parent << endl;
    		break;
    	}
    }


    // trimit la parinte rank-ul minim
    MPI_Send(&deputy, 1, MPI_INT, parent, 2, MPI_COMM_WORLD);

    // primesc de la parinte rank-ul minim
    // daca e egal cu rank-ul meu, at eu sunt leader
    MPI_Recv(&val, 1, MPI_INT, parent, 2, MPI_COMM_WORLD, &status);

    if (val < deputy && val != deputy) {
    	deputy = val;
    }

    // trimit la copii rank-ul minim
    for (unsigned i = 0; i < recv_from.size(); i++) {
    	MPI_Send(&deputy, 1, MPI_INT, recv_from[i], 2, MPI_COMM_WORLD);
    }

    return deputy;
}


int main (int argc, char** argv) {

    int rank, size;
    MPI_Status status;
    int top[CAPACITY][CAPACITY];
    int *neigh;
    int no_of_neigh;
    int parent;
    int eco_count;
    int val = 1;
    int route_vec[CAPACITY];
    vector<int> banned_neigh;

    int mat_size = CAPACITY * CAPACITY;

    // init
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        cout << "Usage: ./tema neigh.in msg.in" << endl;
        MPI_Finalize();
    }

    // fiecare proces citeste din fisier vecinii sai
    string filename(argv[1]);
    neigh = new int[CAPACITY];
    no_of_neigh = readNeighData(rank, filename, neigh);

    char out_rank[10];
	sprintf(out_rank, "%d", rank);

    // Etapa 1: Generare tabela te rutare

    for (int i = 0; i < no_of_neigh; i++) {
    	top[rank][neigh[i]] = 1;
    	top[neigh[i]][rank] = 1;
    }

    // daca sunt initiatorul
    if (rank == 0) {
    	// trimit cereri de topologie
    	val = 1;
    	for (int i = 0; i < no_of_neigh; i++) {
    			MPI_Send(&val, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    	}
    	eco_count = no_of_neigh;

    	recvTopFromNeigh(rank, size, neigh, top, eco_count, mat_size, banned_neigh);

    	// am primit matr cu topologia, acuma calculez tab de rutare si
	    // o trimit mai departe tuturor

	    int routing_table[CAPACITY][CAPACITY];
	    FloydWarshall(top, size, routing_table);

		// Trimit topologia tuturor
	    masterSendTop(rank, size, neigh, routing_table, mat_size, no_of_neigh);

	    //in final obtin vectorul de rutare:
    	for (int i = 0; i < size; i++) {
    		route_vec[i] = routing_table[rank][i];
    	}

    	// scriu in fisier outputul pt ex1:
	    // masterWriteRoutingVec(route_vec, size, rank, out_rank, top);

    }

    // ceilalti incep prin a primi un mesaj de tip "cerere"/"sondaj"
    if (rank != 0) {
    	
    	MPI_Recv(&val, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
    	parent = status.MPI_SOURCE;

    	// trimit cereri copiilor
    	val = 1;
    	for (int i = 0; i < no_of_neigh; i++) {
    		if (neigh[i] != parent) {
    			MPI_Send(&val, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    		}
    	}
    	eco_count = no_of_neigh - 1;

    	recvTopFromNeigh(rank, size, neigh, top, eco_count, mat_size, banned_neigh);

    	// raspund parintelui cu topologia mea (val == 2)
    	val = 2;
    	MPI_Send(&val, 1, MPI_INT, parent, 1, MPI_COMM_WORLD);
    	MPI_Send(top, mat_size, MPI_INT, parent, 1, MPI_COMM_WORLD);

    	// receptioneaza si trimite mai departe top
    	othersSendTop(rank, size, neigh, top, mat_size, no_of_neigh, banned_neigh);

    	//in final obtin vectorul de rutare:
    	for (int i = 0; i < size; i++) {
    		route_vec[i] = top[rank][i];
    	}

    }
    for (int i = 0; i < size; i++) {
        MPI_Barrier(MPI_COMM_WORLD);

        if (i == rank) {
            if (rank == 0) {
               if (rank == 0) {
                    cout << "TOP:\n";
                    // afisez topologia:
                    for (int i = 0; i < size; i++) {
                        for (int j = 0; j < size; j++) {
                            cout << top[i][j] << " ";
                        }
                        cout << endl;
                    }
                }
            }            
            showRoutingVec(route_vec, size, rank);
            cout << endl;
        }
    }


    // Etapa 2:
    // Toate procesele functioneaza la fel, nu mai e diferentiere intre rank
    // si ceilalti
    vector <string> messages;
    vector <int> destination;
    int no_of_messages;
    string msg_filename(argv[2]);

    no_of_messages = readMessagesFile(rank, msg_filename, messages, destination);

    MPI_Request request;
    int flag = 0;
    int active_procs = size;
    int active_procs_v[size];

    for (int i = 0; i < size; i++) {
    	active_procs_v[i] = 1;
    }

    int done = 0;
    int init_bcast = 0;
    

    while (active_procs > 0 || no_of_messages > 0) {
    	int type;
    	

    	MPI_Irecv(&type, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &request);

    	while (1) {
    		sleep(1);
    		MPI_Test(&request, &flag, &status);

    		if (flag == 1) {
    			// am primit cv
    			if (type == 4) {
    				// mesaj normal
    				int dest;
    				int source = status.MPI_SOURCE;
    				char msg[BUF_SIZE];
    				MPI_Recv(&dest, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
    				MPI_Recv(msg, BUF_SIZE, MPI_CHAR, source, 1, MPI_COMM_WORLD, &status);
    				

    				if (dest == rank) {
    					cout << rank << ":: From: " << source << " To: " << dest << " Msg: " << msg << endl;
    				} else {
    					//rutez mesajul
    					//cout << rank << ": rutez mesajul" << endl;
    					type = 4;
    					int next_hop = route_vec[dest];

    					MPI_Send(&type, 1, MPI_INT, next_hop, 1, MPI_COMM_WORLD);
    					MPI_Send(&dest, 1, MPI_INT, next_hop, 1, MPI_COMM_WORLD);
    					MPI_Send(msg, BUF_SIZE, MPI_CHAR, next_hop, 1, MPI_COMM_WORLD);

    					cout << rank << ":: From: " << source << " To: " << dest << " Next Hop: " << next_hop << " Msg: " << msg << endl;
    				}
    			}

    			if (type == 5 || type == 7) {
    				// mesaj de broadCast
    				char msg[BUF_SIZE];
    				int visited[size], visited2[size];
    				int source = status.MPI_SOURCE;
    				MPI_Recv(msg, BUF_SIZE, MPI_CHAR, source, 1, MPI_COMM_WORLD, &status);
    				MPI_Recv(visited, size, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
    				
    				
    				for (int i = 0; i < size; i++) {
    					visited2[i] = visited[i];
    				}
    				for (int i = 0; i < no_of_neigh; i++) {
    					visited2[neigh[i]] = 1;
    				}

    				//type = 5;
    				for (int i = 0; i < no_of_neigh; i++) {
    					if (visited[neigh[i]] == 0) {
	    					MPI_Send(&type, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
	    					MPI_Send(msg, BUF_SIZE, MPI_CHAR, neigh[i], 1, MPI_COMM_WORLD);
	    					MPI_Send(visited2, size, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
	    				}
    				}

                    if (type == 5) {
    				    cout << rank << ":: From: " << source << " BroadCast Msg: " << msg << endl;
                    }

    			}

    			if (type == 6) {
    				// cine mi-a trimis mesajul a terminat de trimis
    				// aici dest are sensul de "cel care a terminat de trim msg"
    				int dest, source;
    				source = status.MPI_SOURCE;

    				MPI_Recv(&dest, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
    				
    				if (active_procs_v[dest] == 1) {
    					active_procs_v[dest] = 0;
    					active_procs --;

    					// trimit si eu mai departe bCast cu inform asta
    					type = 6;
    					for (int i = 0; i < no_of_neigh; i++) {
    						MPI_Send(&type, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    						MPI_Send(&dest, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    					}
    				}
    				
    			}

    			break;
    		} else {
    			// n-am primit nimic inca
    			// trimit eu daca am de trimis cv

                if (init_bcast == 0) {
                    // e mesaj de bCast
                    char msg[BUF_SIZE];
                    sprintf(msg, "%s", "Init bCast");
                    int visited[size];
                    for (int i = 0; i < size; i++) {
                        visited[i] = 0;
                    }
                    visited[rank] = 1;
                    for (int i = 0; i < no_of_neigh; i++) {
                        visited[neigh[i]] = 1;
                    }

                    type = 7;
                    for (int i = 0; i < no_of_neigh; i++) {
                        MPI_Send(&type, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
                        MPI_Send(msg, BUF_SIZE, MPI_CHAR, neigh[i], 1, MPI_COMM_WORLD);
                        MPI_Send(visited, size, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
                    }
                    init_bcast = 1;
                    //cout << rank << ":: Sending init bCast" << endl;
                } else {
        			if (no_of_messages > 0) {
        				int dest, next_hop;
        				char msg[BUF_SIZE];
        				dest = destination[no_of_messages-1];
        				strcpy(msg, messages[no_of_messages-1].c_str());

        				if (dest >= 0) {	
        					// mesaj normal
        					type = 4;
        					next_hop = route_vec[dest];

        					MPI_Send(&type, 1, MPI_INT, next_hop, 1, MPI_COMM_WORLD);
        					MPI_Send(&dest, 1, MPI_INT, next_hop, 1, MPI_COMM_WORLD);
        					MPI_Send(msg, BUF_SIZE, MPI_CHAR, next_hop, 1, MPI_COMM_WORLD);

        					cout << rank << ":: From: " << rank << " To: " << dest << " Next Hop: " << next_hop << " Msg: " << msg << endl;
        					    					
        				} else {
        					// e mesaj de bCast
        					int visited[size];
        					for (int i = 0; i < size; i++) {
        						visited[i] = 0;
        					}
        					visited[rank] = 1;
        					for (int i = 0; i < no_of_neigh; i++) {
        						visited[neigh[i]] = 1;
        					}

        					type = 5;
        					for (int i = 0; i < no_of_neigh; i++) {
        						MPI_Send(&type, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    	    					MPI_Send(msg, BUF_SIZE, MPI_CHAR, neigh[i], 1, MPI_COMM_WORLD);
    	    					MPI_Send(visited, size, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
        					}
        					cout << rank << ":: From: " << rank << " BroadCast Msg: " << msg << endl;
        				}
        				no_of_messages --;
        			} else {
    	    			if (done == 0) {
    	    				// nu mai am de trimis nimic, anunt ca am terminat
    	    				type = 6;
    	    				for (int i = 0; i < no_of_neigh; i++) {
    							MPI_Send(&type, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    							MPI_Send(&rank, 1, MPI_INT, neigh[i], 1, MPI_COMM_WORLD);
    						}
                            //cout << rank << ":: bCast: Done sending messages" << endl;
    						done = 1;
    					} else {
    						//cout << rank << " : NOP\n";
    					}
        			}
                }
    			
    		}

    	}

    }

    int leader = findLeader(rank, no_of_neigh, neigh, banned_neigh);
    int deputy = findDeputy(rank, no_of_neigh, neigh, banned_neigh, leader);

    cout << rank << ":: Leader: " << leader << " Deputy: " << deputy << endl;


	MPI_Finalize();
	return 0;
}
