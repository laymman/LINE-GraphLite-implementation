/**
 * @author  SUN Qian, XIAO Jiawei, ZHANG Yunfei
 * @version 0.1
 * 
 * @email sunqian18s@ict.ac.cn, zhangyunfei18s@ict.ac.cn, zhangyunfei18s@ict.ac.cn
 *
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <map>
#include <gsl/gsl_rng.h>

#include "GraphLite.h"

#define VERTEX_CLASS_NAME(name) LINE##name

#define VERTEX_DIM 200
#define ORDER 2 //input parameter order
#define NEG_NUM 5
#define TOTAL_SAMPLES 1e4
#define RHO 0.025
#define PARALLEL_NUM 40

#define ROOT 0
#define RANGE 5000 // [superstep * RANGE, (superstep+1) * RANGE)

#define NEG_SAMPLING_POWER 0.75
#define NEG_TABLE_SIZE 1e8
#define SIGMOID_BOUND 6
#define SIGMOID_TABLE_SIZE 1000



typedef float real;
typedef enum {START, END, NEG} vertex_type;//start, end or negative vertex
typedef double VertexWeit;
struct VertexMsg{
    int type; // message type: 0,1,2,3
    int64_t source_id; // source vertex of edge
    int64_t target_id; // target vertex of edge
    real rho_m;

    /* Type 0 Message: all vertices send vertex and edge data to root vertex  */
    /* ---------------------------------1------------------------------------ */
    VertexWeit weight;
    /* ---------------------------------------------------------------------- */

    /* Type 1 Message: all vertices send it's degree to root vertex  */
    /* ---------------------------------1------------------------------------ */
    VertexWeit degree;
    /* ---------------------------------------------------------------------- */

    /* Type 2 Message: root sends sample edge and vertex data to related
     * vertices */
    /* ---------------------------------2------------------------------------ */
    // int order; // order of proximity
    int64_t neg_vid[NEG_NUM]; // negative sample vertex array
    /* ---------------------------------------------------------------------- */

    /* Type 3 Message: vertex send vector of vertex to related vertices */
    /* ---------------------------------3------------------------------------ */
    vertex_type source_vertex_type; // type of vertex which sends message
    vertex_type target_vertex_type; // type of vertex which receives message

    real vec_v[VERTEX_DIM];
    /* ---------------------------------------------------------------------- */

};
struct VertexVal{
    real *emb_vertex;
    real *emb_context;
};


class VERTEX_CLASS_NAME(InputFormatter): public InputFormatter {
public:
    int64_t getVertexNum() {
        unsigned long long n;
        sscanf(m_ptotal_vertex_line, "%lld", &n);
        m_total_vertex= n;
        return m_total_vertex;
    }
    int64_t getEdgeNum() {
        unsigned long long n;
        sscanf(m_ptotal_edge_line, "%lld", &n);
        m_total_edge= n;
        return m_total_edge;
    }
    int getVertexValueSize() {
        m_n_value_size = sizeof(VertexVal);
        return m_n_value_size;
    }
    int getEdgeValueSize() {
        m_e_value_size = sizeof(VertexWeit);
        return m_e_value_size;
    }
    int getMessageValueSize() {
        m_m_value_size = sizeof(VertexMsg);
        return m_m_value_size;
    }
    void loadGraph() {
        unsigned long long last_vertex;
        unsigned long long from;
        unsigned long long to;
        VertexWeit weight = 0;
        
        VertexVal value;
//        // init embedding
//        posix_memalign((void **)&(value.emb_vertex), 128, (long long)VERTEX_DIM * sizeof(real));
//        posix_memalign((void **)&(value.emb_context), 128, (long long)VERTEX_DIM * sizeof(real));
//        if (value.emb_vertex == NULL) { printf("Error: memory allocation failed\n"); exit(1); }
//        if (value.emb_context == NULL) { printf("Error: memory allocation failed\n"); exit(1); }
//        for (int b = 0; b < VERTEX_DIM; b++) {
//            emb_vertex[b] = (rand() / (real)RAND_MAX - 0.5) / VERTEX_DIM;
//            emb_context[b] = 0;
//        }

        int outdegree = 0;
        
        const char *line= getEdgeLine();

        // Note: modify this if an edge weight is to be read
        //       modify the 'weight' variable

        sscanf(line, "%lld %lld %lf", &from, &to, &weight);
        addEdge(from, to, &weight);

        last_vertex = from;
        ++outdegree;
        for (int64_t i = 1; i < m_total_edge; ++i) {
            line= getEdgeLine();

            // Note: modify this if an edge weight is to be read
            //       modify the 'weight' variable

            sscanf(line, "%lld %lld %lf", &from, &to, &weight);
            if (last_vertex != from) {
                addVertex(last_vertex, &value, outdegree);
                last_vertex = from;
                outdegree = 1;
            } else {
                ++outdegree;
            }
            addEdge(from, to, &weight);
        }
        addVertex(last_vertex, &value, outdegree);
    }
};

class VERTEX_CLASS_NAME(OutputFormatter): public OutputFormatter {
public:
    void writeResult() {
        int64_t vid;
        char s[1024];
        VertexVal val;

        for (ResultIterator r_iter; ! r_iter.done(); r_iter.next() ) {
            r_iter.getIdValue(vid, &val);
            int n = sprintf(s, "%lld ", (int64_t)vid);
            writeNextResLine(s, n);
            for (int i = 0; i < VERTEX_DIM; ++i) {
                n = sprintf(s, "%lf ", val.emb_vertex[i]);
                writeNextResLine(s, n);
            }
            n = sprintf(s, "\n");
            writeNextResLine(s, n);
        }
    }
};

// An aggregator that records a int value which denote the number of vertex that send message
class VERTEX_CLASS_NAME(Aggregator): public Aggregator<int> {
private:
    int k_val = 0; //input parameter K
public:
    void init() {
        m_global = k_val;
        m_local = 0;
    }
    void* getGlobal() {
        return &m_global;
    }
    void setGlobal(const void* p) {
        m_global = * (int *)p;
    }
    void* getLocal() {
        return &m_local;
    }
    int getKValue() {
        return k_val;
    }
    void setKValue(int k) {
        k_val = k;
    }
    void merge(const void* p) {
        m_global += * (int *)p;
    }
    void accumulate(const void* p) {
        m_local += * (int *)p;
    }
};

class VERTEX_CLASS_NAME(): public Vertex <VertexVal, VertexWeit, VertexMsg> {
private:
    // Parameters for loading graph 
    bool state = 0; // 0 for loading graph; 1 for updating vertex value

    /* Parameters for ROOT only*/
    long long num_edges = 0, num_vertices = 0;
    real init_rho = RHO, rho = RHO;
    long long count = 0, last_count = 0, current_sample_count = 0;
    unsigned long long seed = 1;
    const gsl_rng_type * gsl_T;
    gsl_rng * gsl_r;

    // Parameters for edge sampling
    long long *alias;
    double *prob;

    std::vector<VertexWeit> *edge_weight, *vertex_degree;
    std::vector<int64_t> *edge_source_id, *edge_target_id;
    std::vector<pair<int64_t,VertexWeit>> *vid_map; //{(vid,degree)}
    int *neg_table;
    real *sigmoid_table;

public:
    void compute(MessageIterator* pmsgs) {
        if (* (int *)getAggrGlobal(0) == 1 ) {
            // update signal
            state = 1;
        } else if (* (int *)getAggrGlobal(0) == 2 ) {
            // halt signal
            voteToHalt();
            return ;
        }

        int64_t vid = getVertexId();

        if (state == 0) {
            // load graph
            if (getSuperstep() * RANGE <= vid && vid < (getSuperstep()+1) * RANGE) {
                // malloc for pointer-to
                // std::cout<< vid << ": " 
                //        << "initialize\n"; fflush(stdout);
                InitSigmoidTable();
                VertexVal * val = mutableValue();
                val->emb_vertex = (real *)malloc(VERTEX_DIM*sizeof(real));
                val->emb_context = (real *)malloc(VERTEX_DIM*sizeof(real));
                for (int i = 0; i < VERTEX_DIM; ++i) {
                    val->emb_vertex[i] =  (rand() / (real)RAND_MAX - 0.5) / VERTEX_DIM;
                }
                for (int i = 0; i < VERTEX_DIM; ++i) {
                    val->emb_context[i] = 0;
                }
                if (vid == ROOT) {
                    edge_source_id = new std::vector<int64_t>();
                    edge_target_id = new std::vector<int64_t>();
                    edge_weight = new std::vector<VertexWeit>();
                    vid_map = new std::vector<pair<int64_t,VertexWeit>>();
                }

                //std::cout<< vid << ": " 
                //        << "construct msg\n"; fflush(stdout);
                // vertices in range send edge information to root vertex
                OutEdgeIterator outEdgeIterator = getOutEdgeIterator();
                VertexMsg msg;
                VertexWeit degree = 0;
                msg.type = 0;
                msg.source_id = vid;
                for ( ; ! outEdgeIterator.done(); outEdgeIterator.next() ) {
                    msg.target_id = outEdgeIterator.target();
                    msg.weight = outEdgeIterator.getValue();
                    degree += msg.weight;    
                    sendMessageTo(ROOT, msg);
                }
                msg.type = 1;
                msg.degree = degree;
                sendMessageTo(ROOT, msg);
                // std::cout<< vid << ": " 
                //        << "sent msg\n"; fflush(stdout);
            }
            if (vid == ROOT && getSuperstep() != 0) {
                // root receives messages
                // process message
                long long flag = 0;
                for ( ; ! pmsgs->done(); pmsgs->next() ) {
                    VertexMsg msg = pmsgs->getValue();
                    if (msg.type == 0) {
                        num_edges += 1;
                        edge_source_id->push_back(msg.source_id);
                        edge_target_id->push_back(msg.target_id);
                        edge_weight->push_back(msg.weight);
                    }
                    if (msg.type == 1) {
                        vid_map->push_back(pair<int64_t,VertexWeit>(msg.source_id,msg.degree));
                    }
                    flag++;
                }
                if (flag == 0) {
                    // stop loading graph
                    num_vertices = vid_map->size();
                    // set vid as the index of vertex_degree
                    vertex_degree = new std::vector<VertexWeit>(num_vertices);
                    for (int64_t i = 0; i < num_vertices; ++i) {
                        vertex_degree->at(vid_map->at(i).first) = vid_map->at(i).second;
                    }
                    // free
                    std::vector<pair<int64_t,VertexWeit>>().swap(*vid_map);

                    // do some initializations
                    InitAliasTable();
                    InitNegTable();
                    gsl_rng_env_setup();
                    gsl_T = gsl_rng_rand48;
                    gsl_r = gsl_rng_alloc(gsl_T);
                    gsl_rng_set(gsl_r, 314159265);
                    //std::cout<< vid << ": " 
                    //    << "finish initialization\n"; fflush(stdout);

                    // inform others to stop loading graph
                    int one = 1;
                    accumulateAggr(0, &one);

                    // root vertex samples and sends message
                    for (int i = 0; i < PARALLEL_NUM; ++i) {
                        SampleAndSendMsg();
                    }

                    //std::cout << vid << ": " 
                    //    << "finish state 0\n"; fflush(stdout);
                }
            }
        } else {
            // update vertex value
            if(vid == ROOT){
                //judge for exit
                std::cout << vid << ": " 
                    << "judge for exit\n"; fflush(stdout);
                if (count < TOTAL_SAMPLES / PARALLEL_NUM + 2){
                    if (count - last_count > 10000) {
                        current_sample_count += count - last_count;
                        last_count = count;
                        printf("%cRho: %f  Progress: %.3lf%%", 13, rho, (real)current_sample_count / (real)(TOTAL_SAMPLES + 1) * 100);
                        fflush(stdout);
                        rho = init_rho * (1 - current_sample_count / (real)(TOTAL_SAMPLES + 1));
                        if (rho < init_rho * 0.0001) rho = init_rho * 0.0001;
                    }
                    // root vertex samples and sends message
                    for (int i = 0; i < PARALLEL_NUM; ++i) {
                        SampleAndSendMsg();
                    }
                    count++;
                } else {
                    int two = 2;
                    accumulateAggr(0, &two);
                }
            }

            // process message
            for ( ; ! pmsgs->done(); pmsgs->next() ) {
                //std::cout << vid << ": " 
                //    << "process msg...\n"; fflush(stdout);
                VertexMsg msg = pmsgs->getValue();
                if (msg.type == 2){
                    //std::cout << vid << ": " 
                    //    << "msg type 2\n"; fflush(stdout);
                    // this is a source vertex and need to send it vector to target and neg vertices
                    const VertexVal val = getValue();
                    VertexMsg message;
                    message.type = 3;
                    message.source_id = vid;
                    message.source_vertex_type = START;
                    message.rho_m = msg.rho_m;
                    for (int i = 0; i < VERTEX_DIM; ++i) {
                        message.vec_v[i] = val.emb_vertex[i];
                    }
                    message.target_id = msg.target_id;
                    message.target_vertex_type = END;
                    // send message to end vertex of the edge
                    sendMessageTo(msg.target_id, message);
                    //std::cout << vid << ": " 
                    //    << "send msg to END " << msg.target_id << "\n"; fflush(stdout);
                    // send message to negative vertices of the edge
                    for (int j = 0; j < NEG_NUM; ++j) {
                        message.target_id = msg.neg_vid[j];
                        message.target_vertex_type = NEG;
                        sendMessageTo(msg.neg_vid[j], message);
                        //std::cout << vid << ": " 
                        //    << "send msg to NEG " << msg.neg_vid[j] << "\n"; fflush(stdout);
                    }
                } else if (msg.type == 3){
                    //std::cout << vid << ": " 
                    //    << "msg type 3\n"; fflush(stdout);
                    real vec_error[VERTEX_DIM]={0};
                    if(msg.target_vertex_type == START){
                        //std::cout << vid << ": " 
                        //    << "msg target type START\n"; fflush(stdout);
                        // update start vertex
                        VertexVal * val = mutableValue();
                        for (int i = 0; i < VERTEX_DIM; ++i) {
                            val->emb_vertex[i] += msg.vec_v[i];
                        }
                    } else if(msg.target_vertex_type == END){
                        //std::cout << vid << ": " 
                        //    << "msg target type END\n"; fflush(stdout);
                        // update end vertex
                        VertexVal * val = mutableValue();
                        if (ORDER == 1) Update(msg.vec_v, val->emb_vertex, vec_error, 1, msg.rho_m);
                        if (ORDER == 2) Update(msg.vec_v, val->emb_context, vec_error, 1, msg.rho_m);

                        VertexMsg message;
                        message.type = 3;
                        message.source_id = vid;
                        message.source_vertex_type = END;
                        for (int i = 0; i < VERTEX_DIM; ++i) {
                        	message.vec_v[i] = vec_error[i];
                        }
                        message.target_id = msg.source_id;
                        message.target_vertex_type = START;
                        // send message to start vertex of the edge
                        sendMessageTo(msg.source_id, message);
                        //std::cout << vid << ": " 
                        //    << "send msg to START " << msg.source_id << "\n"; fflush(stdout);

                    } else if(msg.target_vertex_type == NEG){
                        //std::cout << vid << ": " 
                        //    << "msg target type NEG\n"; fflush(stdout);
                        // update negative vertex
                        VertexVal * val = mutableValue();
                        if (ORDER == 1) Update(msg.vec_v, val->emb_vertex, vec_error, 0, msg.rho_m);
                        if (ORDER == 2) Update(msg.vec_v, val->emb_context, vec_error, 0, msg.rho_m);
                        VertexMsg message;
                        message.type = 3;
                        message.source_id = vid;
                        message.source_vertex_type = NEG;
                        for (int i = 0; i < VERTEX_DIM; ++i) {
                        	message.vec_v[i] = vec_error[i];
                        }
                        message.target_id = msg.source_id;
                        message.target_vertex_type = START;
                        // send message to start vertex of the edge
                        sendMessageTo(msg.source_id, message);
                        //std::cout << vid << ": " 
                        //    << "send msg to START " << msg.source_id << "\n"; fflush(stdout);
                    }
                }
            }
        }
    }
    /* root vertex samples and sends message to vertex*/
    void SampleAndSendMsg(){
        int64_t u, v;
        long long curedge = SampleAnEdge(gsl_rng_uniform(gsl_r), gsl_rng_uniform(gsl_r));
        u = edge_source_id->at(curedge);
        v = edge_target_id->at(curedge);
        VertexMsg msg;
        msg.type = 2;
        msg.source_id = u;
        msg.target_id = v;
        // msg.order = ORDER;
        msg.rho_m = rho;
        // NEGATIVE SAMPLING
        for (int d = 0; d < NEG_NUM; d++) {
            msg.neg_vid[d] = neg_table[Rand(seed)];
        }
        sendMessageTo(u, msg);
        // std::cout<< "send msg to " << u << "\n"; fflush(stdout);
    }

	/* The alias sampling algorithm, which is used to sample an edge in O(1) time. */
    void InitAliasTable() {
    	alias = (long long *)malloc(num_edges*sizeof(long long));
    	prob = (double *)malloc(num_edges*sizeof(double));
    	if (alias == NULL || prob == NULL) {
    		printf("Error: memory allocation failed!\n");
    		exit(1);
    	}
    	double *norm_prob = (double *)malloc(num_edges*sizeof(double));
    	long long *large_block = (long long *)malloc(num_edges*sizeof(long long));
    	long long *small_block = (long long *)malloc(num_edges*sizeof(long long));
    	if (norm_prob == NULL || large_block == NULL || small_block == NULL) {
    		printf("Error: memory allocatiion failed!\n");
    		exit(1);
    	}

    	double sum = 0;
    	long long cur_small_block, cur_large_block;
    	long long num_small_block = 0, num_large_block = 0;

    	for (long long k = 0; k != num_edges; k++) sum += edge_weight->at(k);
    	for (long long k = 0; k != num_edges; k++) norm_prob[k] = edge_weight->at(k) * num_edges / sum;

    	for (long long k = num_edges - 1; k >= 0; k--) {
    		if (norm_prob[k] < 1)
    			small_block[num_small_block++] = k;
    		else
    			large_block[num_large_block++] = k;
    	}

    	while (num_small_block && num_large_block) {
    		cur_small_block = small_block[--num_small_block];
    		cur_large_block = large_block[--num_large_block];
    		prob[cur_small_block] = norm_prob[cur_small_block];
    		alias[cur_small_block] = cur_large_block;
    		norm_prob[cur_large_block] = norm_prob[cur_large_block] + norm_prob[cur_small_block] - 1;
    		if (norm_prob[cur_large_block] < 1)
    			small_block[num_small_block++] = cur_large_block;
    		else
    			large_block[num_large_block++] = cur_large_block;
    	}

    	while (num_large_block) prob[large_block[--num_large_block]] = 1;
    	while (num_small_block) prob[small_block[--num_small_block]] = 1;

    	free(norm_prob);
    	free(small_block);
    	free(large_block);
    }
    
    long long SampleAnEdge(double rand_value1, double rand_value2) {
        long long k = (long long)num_edges * rand_value1;
        return rand_value2 < prob[k] ? k : alias[k];
    }

    /* Sample negative vertex samples according to vertex degrees */
    void InitNegTable() {
        double sum = 0, cur_sum = 0, por = 0;
        int vid = 0;
        neg_table = (int *)malloc(NEG_TABLE_SIZE * sizeof(int));
        for (int k = 0; k != num_vertices; k++) sum += pow(vertex_degree->at(k), NEG_SAMPLING_POWER);
        for (int k = 0; k != NEG_TABLE_SIZE; k++)
        {
            if ((double)(k + 1) / NEG_TABLE_SIZE > por)
            {
                cur_sum += pow(vertex_degree->at(vid), NEG_SAMPLING_POWER);
                por = cur_sum / sum;
                vid++;
            }
            neg_table[k] = vid - 1;
        }
    }

    /* Fastly compute sigmoid function */
    void InitSigmoidTable(){
        real x;
        sigmoid_table = (real *)malloc((SIGMOID_TABLE_SIZE + 1) * sizeof(real));
        for (int k = 0; k != SIGMOID_TABLE_SIZE; k++)
        {
            x = 2.0 * SIGMOID_BOUND * k / SIGMOID_TABLE_SIZE - SIGMOID_BOUND;
            sigmoid_table[k] = 1 / (1 + exp(-x));
        }
    }
    real FastSigmoid(real x){
        if (x > SIGMOID_BOUND) return 1;
        else if (x < -SIGMOID_BOUND) return 0;
        int k = (x + SIGMOID_BOUND) * SIGMOID_TABLE_SIZE / SIGMOID_BOUND / 2;
        return sigmoid_table[k];
    }

    /* Update embeddings */
    void Update(real *vec_u, real *vec_v, real *vec_error, int label, real rho_m){
        real x = 0, g;
        for (int c = 0; c != VERTEX_DIM; c++) x += vec_u[c] * vec_v[c];
        g = (label - FastSigmoid(x)) * rho_m;
        for (int c = 0; c != VERTEX_DIM; c++) vec_error[c] += g * vec_v[c];
        for (int c = 0; c != VERTEX_DIM; c++) vec_v[c] += g * vec_u[c];
    }

    /* Fastly generate a random integer */
    int Rand(unsigned long long &seed) {
        seed = seed * 25214903917 + 11;
        int mod = NEG_TABLE_SIZE;
        return (seed >> 16) % mod;
    }
};

class VERTEX_CLASS_NAME(Graph): public Graph {
public:
    VERTEX_CLASS_NAME(Aggregator)* aggregator;

public:
    // argv[0]: PageRankVertex.so
    // argv[1]: <input path>
    // argv[2]: <output path>
    void init(int argc, char* argv[]) {

        setNumHosts(2);
        setHost(0, "localhost", 1411);
        setHost(1, "localhost", 1421);
        // setHost(2, "localhost", 1431);
        // setHost(3, "localhost", 1441);
        // setHost(4, "localhost", 1451);
        // setHost(5, "localhost", 1461);

        if (argc < 3) {
           printf ("Usage: %s <input path> <output path>\n", argv[0]);
           exit(1);
        }

        m_pin_path = argv[1];
        m_pout_path = argv[2];

        aggregator = new VERTEX_CLASS_NAME(Aggregator)[1];
        regNumAggr(1);
        regAggr(0, &aggregator[0]);
    }

    void term() {
        delete[] aggregator;
    }
};

/* STOP: do not change the code below. */
extern "C" Graph* create_graph() {
    Graph* pgraph = new VERTEX_CLASS_NAME(Graph);

    pgraph->m_pin_formatter = new VERTEX_CLASS_NAME(InputFormatter);
    pgraph->m_pout_formatter = new VERTEX_CLASS_NAME(OutputFormatter);
    pgraph->m_pver_base = new VERTEX_CLASS_NAME();

    return pgraph;
}

extern "C" void destroy_graph(Graph* pobject) {
    delete ( VERTEX_CLASS_NAME()* )(pobject->m_pver_base);
    delete ( VERTEX_CLASS_NAME(OutputFormatter)* )(pobject->m_pout_formatter);
    delete ( VERTEX_CLASS_NAME(InputFormatter)* )(pobject->m_pin_formatter);
    delete ( VERTEX_CLASS_NAME(Graph)* )pobject;
}