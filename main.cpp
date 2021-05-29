#include <iostream>
#include <pthread.h>
#include <vector>
#include <iomanip>

#define CITY_MAX_NUM  20
#define OUTPUT_WIDE 3
#define CPU_NUM 8
using namespace std;

bool isNumber(char num) {
    return num >= '0' && num <= '9';
}

int convertNum(char num) {
    return num - '0';
}

void output();

int diagram[CITY_MAX_NUM][CITY_MAX_NUM];
int city_num;
int divide_time = 0;

/*************************************************************/
/****************  Data Structure Definition  ****************/
/*************************************************************/
struct tour_t {
    int tour[CITY_MAX_NUM];
    int len;
    int distance;

    tour_t() {
        for (int i = 0; i < CITY_MAX_NUM; ++i) {
            tour[i] = 0;
        }
        len = 0;
        distance = 0;
    }

    void anneal() {
        distance = distance - diagram[len - 2][len - 1];
        len--;
        tour[len] = INT_MAX;
    }

    bool contain_city(int city) {
        for (int i = 0; i < len; ++i) {
            if (tour[i] == city) return true;
        }
        return false;
    }

    tour_t *copy_and_insert(int city) {
        tour_t *tmp = this->deep_copy();
        if (len == 0) {
            tmp->tour[0] = city;
            tmp->distance = diagram[0][city];
            tmp->len = 1;
        } else {
            int cur_city = tmp->tour[tmp->len - 1];
            tmp->tour[tmp->len] = city;
            if (diagram[cur_city][city] == INT_MAX) {
                tmp->distance = INT_MAX;
            } else { tmp->distance = tmp->distance + diagram[cur_city][city]; }
            tmp->len++;
        }
        return tmp;
    }

    void append_city(int city) {
        tour[len] = city;
        distance = distance + diagram[get_cur_city()][city];
        len++;
    }

    tour_t *deep_copy() {
        tour_t *tmp = new tour_t();
        for (int i = 0; i < city_num; ++i) {
            tmp->tour[i] = this->tour[i];
        }
        tmp->len = this->len;
        tmp->distance = this->distance;
        return tmp;
    }

    int get_cur_city() {
        if (len == 0) return 0;
        return tour[len - 1];
    }
};


struct node_t {
    tour_t *tour;
    node_t *prev;
    node_t *next;

    node_t(tour_t *tour) {
        this->tour = tour;
        prev = nullptr;
        next = nullptr;
    }

    node_t *deep_copy() {
        node_t *tmp = new node_t(this->tour->deep_copy());
        tmp->prev = this->prev;
        tmp->next = this->next;
        return tmp;
    }
};

struct tour_stack_t {
    node_t *head;
    node_t *tail;
    int length;

    tour_stack_t() {
        this->head = nullptr;
        this->tail = nullptr;
        this->length = 0;
    }

    bool is_empty() {
        return (this->head == nullptr);
    }

    node_t *pop_bottom_element() {
        if (this->is_empty()) { return nullptr; }
        else if (length == 1) {
            this->head = nullptr;
            this->tail = nullptr;
            length--;
            return head;
        } else {
            auto res = head;
            head=head->next;
            head->prev = head;
            res->prev = nullptr;
            res->next = nullptr;
            length--;
            return res;
        }
    }

    void push(node_t *node) {
        if (this->is_empty()) {
            head = node;
            head->prev = head;
            tail = node;
            tail->prev = head;
            length++;
        } else if (length == 1) {
            head->next = node;
            tail = node;
            tail->prev = head;
            tail->next = nullptr;
            length++;
        } else {
            tail->next = node;
            node->prev = tail;
            tail = tail->next;
            length++;
        }
    }

    void push(tour_t *tour) {
        node_t *node = new node_t(tour);
        push(node);
    }

    void pop() {
        if (is_empty()) { return; }
        if (length == 1) {
            head = nullptr;
            tail = nullptr;
        } else {
            node_t *tmp = tail;
            tail = tail->prev;
            tail->next = nullptr;
            free(tmp);
        }
        length--;

    }

    node_t *peek() {
        if (is_empty()) return nullptr;
        return this->tail->deep_copy();
    }
};

struct tour_queue_t {
    node_t *head;
    node_t *tail;
    int length;

    tour_queue_t() {
        head = nullptr;
        tail = nullptr;
        length = 0;
    }

    bool is_empty() {
        return this->length == 0;
    }

    void push(tour_t *tour) {
        node_t *node = new node_t(tour);
        push(node);
    }

    void push(node_t *node) {
        if (this->is_empty()) {
            head = node;
            head->prev = head;
            tail = node;
            tail->prev = head;
            length++;
        } else if (length == 1) {
            head = node;
            head->prev = head;
            head->next = tail;
            tail->prev = head;
            length++;
        } else {
            auto p = head;
            head = node;
            head->prev = head;
            head->next = p;
            p->prev = head;
            length++;
        }
    }

    void pop() {
        if (is_empty()) { return; }
        if (length == 1) {
            head = nullptr;
            tail = nullptr;
        } else {
            node_t *tmp = tail;
            tail = tail->prev;
            tail->next = nullptr;
            free(tmp);
        }
        length--;
    }

    node_t *peek() {
        if (is_empty()) return nullptr;
        return this->tail->deep_copy();
    }

};


/****************************************************/
/****************  Global Variables  ****************/
/****************************************************/

/*
 * 与TSP相关的全局变量
 * */
tour_t *best_tour = new tour_t();

/*
 * 与并行相关的全局变量
 * */
pthread_mutex_t mutex_best_tour;
pthread_mutex_t mutex_thread_cmd;
int thread_in_con_wait;
int thread_cout;
pthread_t *thread_handles;
volatile int active_thread_num;
bool running = false;

/*****************************************************/
/****************  Packaged Function  ****************/
/*****************************************************/

void add_thread_num(pthread_t thread) {
    active_thread_num++;
    cout << "Thread NO." << active_thread_num << " has created. Thread id : " << thread <<"\t\t"<<active_thread_num<<" threads remains" << endl;
}
void add_thread_num_new_divided(pthread_t thread){
    active_thread_num++;
    divide_time++;
    cout<<"Divided mission. new thread: "<<thread<<"\t\t"<<active_thread_num<<" threads remains"<<endl;
}

void sub_thread_num() {
    active_thread_num--;
    cout << "One Thread has finished its job. "<<"\t\t\t\t" << active_thread_num << "threads remains" << endl;
}

volatile tour_t* divided_tour;
pthread_cond_t cond_divide_task;
pthread_mutex_t mutex_divide_task;
void *tsp(void *tour) {
    tour_stack_t *stack = new tour_stack_t();
    tour_t *base_tour = (tour_t *) tour;
    node_t *root_city = new node_t(base_tour);
    stack->push(root_city);
    while (!stack->is_empty()) {
        /*
         * CPU负载均衡，当活跃线程数比总线程数少时，获取栈底元素（运算量最大的一个tour）新建tsp线程。
         * 此处也可以自定分割任务的时机，比如当栈的总元素小于10个时才进行分割，减小分割运算量。
         * */
        if(active_thread_num < thread_cout && running){
            if(stack->length>10){
                pthread_mutex_lock(&mutex_thread_cmd);
                //等待锁的过程总可能已经有线程修改了线程总数
                if(active_thread_num < thread_cout){
                    tour_t *tour = stack->pop_bottom_element()->tour;
                    if (tour != nullptr) {
                        pthread_t thread;
                        pthread_create(&thread, NULL, tsp, tour);
                        add_thread_num_new_divided(thread);
                        pthread_join(thread, NULL);
                    }
                }
                pthread_mutex_unlock(&mutex_thread_cmd);
            }
        }
        node_t *head = stack->peek();
        stack->pop();
        tour_t *cur_tour = head->tour;
        //区分走回原点的事件，这里是递归出口
        if (cur_tour->len == city_num - 1) {
            //走完所有城市，回到起点,此时需要判断是否需要更新
            if (diagram[cur_tour->get_cur_city()][0] != INT_MAX
                && diagram[cur_tour->get_cur_city()][0] + cur_tour->distance < best_tour->distance) {
                pthread_mutex_lock(&mutex_best_tour);
                //等待锁的过程总可能有线程修改了best_tour，此处再次判定
                if (diagram[cur_tour->get_cur_city()][0] != INT_MAX
                    && diagram[cur_tour->get_cur_city()][0] + cur_tour->distance < best_tour->distance) {
                    tour_t *tmp = best_tour;
                    best_tour = cur_tour->copy_and_insert(0);
                    free(tmp);
                }
                //此处释放锁
                pthread_mutex_unlock(&mutex_best_tour);
            }
        } else {
            for (int i = city_num - 1; i > 0; i--) {
                //剪枝已达城市：已经到过的城市，不应当再回去，
                if (cur_tour->contain_city(i)) {
                    continue;
                }
                //剪枝不可达城市：当前城市与前一个城市距离为无限大
                if (diagram[cur_tour->get_cur_city()][i] == INT_MAX) continue;
                //剪枝高累计距离路线：移动到当前城市之后的累计距离比已有的最短距离大
                int new_distance = cur_tour->distance + diagram[cur_tour->get_cur_city()][i];
                if (new_distance > best_tour->distance) continue;
                //当前城市可达，且距离小于已有最短距离
                stack->push(cur_tour->copy_and_insert(i));


            }
        }
        free(cur_tour);
    }
    sub_thread_num();
}

tour_queue_t *queue = new tour_queue_t();

void divide(tour_t *base_tour, int task_num) {

    node_t *root_city = new node_t(base_tour);
    queue->push(root_city);
    bool ready = false;
    while (!queue->is_empty() && !ready) {
        node_t *head = queue->peek();
        queue->pop();
        tour_t *cur_tour = head->tour;
        //区分走回原点的事件，这里是递归出口
        if (cur_tour->len == city_num - 1) {
            //走完所有城市，回到起点,此时需要判断是否需要更新
            if (diagram[cur_tour->get_cur_city()][0] != INT_MAX
                && diagram[cur_tour->get_cur_city()][0] + cur_tour->distance < best_tour->distance) {
                pthread_mutex_lock(&mutex_best_tour);
                //等待锁的过程总可能有线程修改了best_tour，此处再次判定
                if (diagram[cur_tour->get_cur_city()][0] != INT_MAX
                    && diagram[cur_tour->get_cur_city()][0] + cur_tour->distance < best_tour->distance) {
                    tour_t *tmp = best_tour;
                    best_tour = cur_tour->copy_and_insert(0);
                    free(tmp);
                }
                //此处释放锁
                pthread_mutex_unlock(&mutex_best_tour);
            }
        } else {
            for (int i = city_num - 1; i > 0; i--) {
                //剪枝已达城市：已经到过的城市，不应当再回去，
                if (cur_tour->contain_city(i)) {
                    continue;
                }
                //剪枝不可达城市：当前城市与前一个城市距离为无限大
                if (diagram[cur_tour->get_cur_city()][i] == INT_MAX) continue;
                //剪枝高累计距离路线：移动到当前城市之后的累计距离比已有的最短距离大
                int new_distance = cur_tour->distance + diagram[cur_tour->get_cur_city()][i];
                if (new_distance > best_tour->distance) continue;
                //当前城市可达，且距离小于已有最短距离
                queue->push(cur_tour->copy_and_insert(i));

                if (queue->length >= task_num) {
                    ready = true;
                    break;
                }
            }
        }
        free(cur_tour);
    }

}

void input() {

    for (auto &i : diagram) {
        for (int &j : i) {
            j = INT_MAX;
        }
    }
    int m, n, d;
    cin >> city_num;
    int lineNum;
    cin >> lineNum;
    while (--lineNum) {
        cin >> m;
        cin >> n;
        cin >> d;
        diagram[m][n] = d;
    }
    cout << endl;
    cout << "----------------------" << endl;
    cout << "Distance Diagram is :" << endl;
    cout << "----------------------" << endl;


    for (int i = 0; i < city_num; ++i) {
        for (int j = 0; j < city_num; ++j) {
            if (diagram[i][j] == INT_MAX) {
                cout << setw(OUTPUT_WIDE) << "-";
            } else { cout << setw(OUTPUT_WIDE) << diagram[i][j]; }
        }
        cout << endl;
    }
    cout << "----------------------" << endl;
    cout << "----------------------" << endl;


}

void output() {
    cout << "----------------------" << endl;
    cout << "----------------------" << endl;
    cout << "The Shortest Route Is :" << endl;
    cout << "\t0";
    for (int i = 0; i < city_num - 1; ++i) {
        cout << "->" << best_tour->tour[i];
    }
    cout << "->0" << endl;
    cout << "Distance: " << endl << '\t' << best_tour->distance<<endl;
}

void init() {
    active_thread_num = 0;
    best_tour->distance = INT_MAX;
}

void *thread_handler() {

}


void divide_and_conquer() {
    divide(new tour_t(), CPU_NUM);
    thread_cout = queue->length;
    thread_handles = static_cast<pthread_t *>(malloc(thread_cout * sizeof(pthread_t)));
    cout << "Divided into " << thread_cout << " Tasks" << endl;
    cout << "----------------------" << endl;
    cout << "----------------------" << endl;
    for (int i = 0; i < thread_cout; ++i) {
        tour_t *tour = queue->peek()->tour;
        queue->pop();
        pthread_create(&thread_handles[i], NULL, tsp, tour);
        add_thread_num(thread_handles[i]);
    }
    running = true;
    for (int i = 0; i < thread_cout; ++i) {
        pthread_join(thread_handles[i], NULL);
    }
}

int main() {
    input();
    init();
    divide_and_conquer();
//    tsp(new tour_t());
    output();
    cout<<divide_time<<" times balance function called"<<endl;
    return 0;
}
