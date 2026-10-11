#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DISH DICTIONARY */
#define DICT_SIZE 2003
#define CLAUSE 20000

typedef struct Word {        /* dish */
    int pos;
    int next;
    int id;
} Word;

int *dictionary;       /* hash dictionary */
Word *dpool;           /* dishes pool */
int pool_count = 1;

int dish_count = 0;             /* from 1 (++) */
int dish_pos = 0;
int dishes_len = 2048;  

/* LITERALS, CLAUSES */
int *literals;
int lit_len = 1024;
int lit_count = 0;              /* uno di più dei letterali, per il terminale 0 */

int *starts;
int starts_len = 1024;
int starts_count = 0;           /* from 1 (++) */

/* TWL */
int *watch1, *watch2;

/* ADLIST */
int *head;
int *next1;
int *next2;

/* CDCL */
int *val;
int *reason;
int *level;

int *seen;
int *learnt;

int *trail;
int trh, trq;

double *activity;
int *heap, *heap_pos;
int heap_size = 0;
double var_inc = 1.0;
double var_decay = 1.05;
double rescale_threshold = 1e100;

int dec_level;
int len;


/* FUNCTIONS */
unsigned long hash (const char *str);
int readd(char **dishes, char *dish);
int readp(char *dishes, int **literals, int **starts, char *buffer);
int get_id(const char *dishes, const char *name);

void twl(int *watch1, int *watch2);
void madlist(int *head, int *next1, int *next2);

int cdcl();
int unit();
int propagate();
int enqueue(int id, int cl);
int dequeue();
int conflict(int cl);
int learn(int len);
void backtrack(int bjl);
void restore(int mem);

void hswap(int i, int j);
void hup(int pos);
void vbump(int var);
void vdecay();
void vinsert(int var);
int vlazy();




int main() {
    /* dictionary & pool initial setup */
    dictionary = malloc(DICT_SIZE * sizeof(int));
    memset(dictionary, -1, DICT_SIZE * sizeof(int));
    dpool = malloc(5000 * sizeof(Word));


    /* reading all dishes */
    char dish[128];
    char *dishes = (char *)malloc(dishes_len * sizeof(char));

    while (readd(&dishes, dish));

    /* reading all preferences */
    literals = malloc(lit_len * sizeof(int));             /* array of literals */
    starts = malloc(starts_len * sizeof(int));            /* array of positions */

    readp(dishes, &literals, &starts, dish);

    /* dictionary & pool FREE */
    free(dictionary);
    free(dpool);
    free(dishes);

    /* two whatched literals */
    watch1 = calloc(starts_len, sizeof(int));
    watch2 = calloc(starts_len, sizeof(int));

    twl(watch1, watch2);

    /* adlist */
    head = calloc(2*dish_count+1, sizeof(int));
    next1 = calloc(starts_len, sizeof(int));
    next2 = calloc(starts_len, sizeof(int));

    madlist(head, next1, next2);


    /* cdcl structures  */
    val = malloc((dish_count+1) * sizeof(int));
    reason = malloc((dish_count+1) * sizeof(int));
    level = malloc((dish_count+1) * sizeof(int));
    
    trail = malloc((dish_count+1) * sizeof(int));
    seen = malloc((dish_count+1) * sizeof(int));
    learnt = malloc((dish_count+1) * sizeof(int));

    /* vsids structs */
    activity = malloc((dish_count+1) * sizeof(double));
    heap = malloc((dish_count+1) * sizeof(int));
    heap_pos = malloc((dish_count+1) * sizeof(int));

    for (int i = 1; i <= dish_count; i++) {
        activity[i] = 0.0;
        heap[i] = i;
        heap_pos[i] = i;
    }
    heap_size = dish_count;
        
        


    int starts_mem = starts_count;

    /* cdcl */
    int c = cdcl();
    if(c) {
        printf("OK\n");
        return 0;
    }
    
    int k = 1;
    printf("KO\n");
    while(--starts_mem > 0) {
        restore(starts_mem);
        printf("-%d\n", k);

        c = cdcl();
        if(c) {
            printf("OK\n"); 
            return 0;
        }

        k++;
    }



}


/* read dishes */
int readd(char **dishes, char *dish) {
    int c = getchar_unlocked();
    if (c == '\n') return 0;

    char *diishes = *dishes;
    char *dish_start = dish;
    int char_count = 1;

    while (c > ' ') {
        *dish++ = (char)c;
        c = getchar_unlocked();
        char_count++;
    }

    *dish = '\0';

    while ((dish_pos + char_count) > dishes_len)  {
        diishes = realloc(diishes, (2*dishes_len) * sizeof(char));
        dishes_len *= 2;
    }

    dish = dish_start;
    strcpy(&diishes[dish_pos], dish);

    unsigned long h = hash(dish);

    int id = pool_count++;
    dpool[id].pos = dish_pos;
    dpool[id].id = ++dish_count;
    dpool[id].next = dictionary[h];
    dictionary[h] = id;
    dish_pos += char_count;

    *dishes = diishes;

    if (c == '\n') return 0;
    return 1;
}

/* read preferences */
int readp(char *dishes, int **literals, int **starts, char *buffer) {
    int c = getchar_unlocked();
    char *buffer_start = buffer;

    int *staarts = *starts;
    int *literaals = *literals;

    while (c != EOF) {
        if (starts_count+1 >= (starts_len)) {
            staarts = realloc(staarts, (2*starts_len) * sizeof(int));
            starts_len *= 2;
        }

        staarts[++starts_count] = lit_count;

        while (c != '\n') {
            while (c > ' ') {
                *buffer++ = c;
                c = getchar_unlocked();
            }

            *buffer = '\0';
            buffer = buffer_start;
            int id = get_id(dishes, buffer);


            if ((lit_count+1) >= lit_len) {
                literaals = realloc(literaals, (2*lit_len) * sizeof(int));
                lit_len *= 2;
            }

            literaals[lit_count] = id;
            lit_count++;
            
            if (c == '\n') break;
            c = getchar_unlocked();
        }

        literaals[lit_count] = 0;
        lit_count++;

        c = getchar_unlocked();
    }

    *starts = staarts;
    *literals = literaals;

    return 0;
}

/* get dish ID */
int get_id(const char *dishes, const char *name) {
    int sign = 1;

    if (name[0] == '-') {
        sign = -1;
        name++;
    }

    unsigned long h = hash(name);
    int id = dictionary[h];

    while (id != -1) {
        if (strcmp(name, &dishes[dpool[id].pos]) == 0)
            return sign * dpool[id].id;

        id = dpool[id].next;
    }
    
    return 0;
}

/* hash function */
unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    
    return hash % DICT_SIZE;
}

/* twl */
void twl(int *watch1, int *watch2) {
    for(int i = 1; i <= starts_count; i++) {
        int start = starts[i];
        watch1[i] = literals[start++];
        watch2[i] = literals[start];
    }
}

/* adlist */
void madlist(int *head, int *next1, int *next2) {
    for (int i = 1; i <= starts_count; i++) {
        int id1, id2;
        id1 = watch1[i];
        id2 = watch2[i];

        if (id1 < 0) id1 = -id1 + dish_count;
        next1[i] = head[id1];
        head[id1] = i;

        if (id2 == 0) continue;
        else if (id2 < 0) id2 = -id2 + dish_count;
        next2[i] = head[id2];
        head[id2] = i;
    }
}

/* cdcl */
int cdcl() {
    /* init */
    dec_level = 0;
    trh = 0; trq = 0;

    memset(val, 0, (dish_count+1) * sizeof(int));
    memset(reason, 0, (dish_count+1) * sizeof(int));
    memset(level, -1, (dish_count+1) * sizeof(int));
    memset(trail, 0, (dish_count+1) * sizeof(int));

    if (unit()) return 0;
    
    while(1) {
        int conf = propagate();
        if (conf != 0) {
            if (dec_level == 0) return 0;

            int bjl = conflict(conf);
            int lcl = starts_count+1;

            learn(len-1);
            backtrack(bjl);
            enqueue(learnt[0], lcl);
        }
        else {
            if (trh == dish_count) return 1; /* sat */

            int nlit = vlazy();
            if (nlit == 0) return 1;

            dec_level++;
            enqueue(nlit, 0);
        }
    }
    

}

int unit() {
    int id;
    int absid;
    int v;

    for (int i = 1; i <= starts_count; i++) {
        if(watch2[i] == 0) {
            id = watch1[i];
            absid = abs(id);
            if(id < 0) v = -1;
            else v = 1;

            if (val[absid] != 0) {
                if (val[absid] != v) return i;
                else continue;
            }
                
            
            val[absid] = v;
            reason[abs(id)] = i;
            level[abs(id)] = 0;

            trail[trh++] = id;
        }
    }

    return 0;
}

int propagate() {
    int id, idd;
    int cl;

    while (trq < trh) {
        id = dequeue();
        idd = -id;

        int p = (idd < 0) ?  -idd + dish_count : idd;
         

        int curr = head[p];
        int prev = 0;

        while (curr != 0) {
            cl = curr;

            int uno = (watch1[cl] == idd);
            int next = uno ? next1[cl] : next2[cl];
            int other = uno ? watch2[cl] : watch1[cl];

            if ((val[abs(other)] * other) > 0) {
                prev = curr;
                curr = next;
                continue;
            }

            int start = starts[cl] - 1;
            int new = 0;

            while (literals[++start]) {
                int lit = literals[start];
                if (lit == watch1[cl] || lit == watch2[cl])
                    continue;
                
                if (val[abs(lit)] == 0 || (val[abs(lit)] * lit) > 0) {
                    new = lit;
                    break;
                }
            }

            if (new != 0) {
                if (prev == 0) head[p] = next;
                else {
                    if (watch1[prev] == idd) 
                        next1[prev] = next;
                    else 
                        next2[prev] = next;
                }

                int newp; 
                if (new < 0) newp = -new + dish_count;
                else newp = new;

                if (uno) {
                    watch1[cl] = new;
                    next1[cl] = head[newp];
                }
                else {
                    watch2[cl] = new;
                    next2[cl] = head[newp];
                }
                head[newp] = cl;

                curr = next;
            }
            else {
                if (val[abs(other)] == 0)
                    enqueue(other, cl);
                else
                    return cl;
                
                prev = curr;
                curr = next;
            }
        }
    }

    return 0;
}

int enqueue(int id, int cl) {
    trail[trh] = id;
    reason[abs(id)] = cl;
    level[abs(id)] = dec_level;
    val[abs(id)] = (id > 0) ? 1 : -1;

    trh++;

    return 0;
}

int dequeue() {
    int id = trail[trq];
    
    trq++;

    return id; 
}

int conflict(int cl) {
    int counter = 0;
    int start;
    int idx;
    int lit;
    int var;
    int p;

    memset(seen, 0, (dish_count+1) * sizeof(int));
    memset(learnt, 0, (dish_count+1) * sizeof(int));

    start = starts[cl];
    idx = trh-1;
    len = 1;
    p = 0;

    do {
        start = starts[cl];
        while (literals[start]) {
            lit = literals[start];
            var = abs(lit);

            if (var == abs(p)) {
                start++; 
                continue;
            }

            if (!seen[var] && level[var] > 0) {
                seen[var] = 1;

                vbump(var);

                if (level[var] == dec_level)
                    counter++;
                else {
                    int llit;
                    llit = (val[var] > 0) ? -var : var;
                    learnt[len++] = llit;
                }
            }

            start++;
        }

        while (idx >= 0 && !seen[abs(trail[idx])]) idx--;

        p = trail[idx];
        var = abs(p);
        idx--;

        seen[var] = 0; 
        counter--; 

        if (counter > 0)
            cl = reason[var];
    } while (counter > 0);


    learnt[0] = (val[abs(p)] > 0) ? -abs(p) : abs(p);
    learnt[len++] = 0;


    int bjl = 0;
    if (len == 2) {
        vdecay();
        return bjl;
    }

    int i = 1;
    while(learnt[i]) {
        if (level[abs(learnt[i])] > bjl)
            bjl = level[abs(learnt[i])];
        i++;
    }

    for (i = 1; i < len; i++) {
        if (level[abs(learnt[i])] == bjl) {
            int swap = learnt[1];
            learnt[1] = learnt[i];
            learnt[i] = swap;
            break;
        }
    }

    vdecay();

    return bjl;
}

int learn(int len) {
    int i; 
    int cl = starts_count + 1;

    /* realloc if needed */
    while((lit_count + len + 1) >= lit_len) {
        lit_len *= 2;
        literals = realloc(literals, lit_len * sizeof(int));
    }

    if((starts_count+1) >= starts_len) {
        starts_len *= 2;
        starts = realloc(starts, starts_len * sizeof(int));
        watch1 = realloc(watch1, starts_len * sizeof(int));
        watch2 = realloc(watch2, starts_len * sizeof(int));
        next1 = realloc(next1, starts_len * sizeof(int));
        next2 = realloc(next2, starts_len * sizeof(int));
    }

    starts[++starts_count] = lit_count;

    i = 0; 
    while(learnt[i])
        literals[lit_count++] = learnt[i++];
    literals[lit_count++] = 0;

    watch1[cl] = learnt[0];
    watch2[cl] = learnt[1];

    int p1 = (watch1[cl] > 0) ? watch1[cl] : (-watch1[cl] + dish_count);
    next1[cl] = head[p1];
    head[p1] = cl;

    if (watch2[cl] == 0) return 1;
    int p2 = (watch2[cl] > 0) ? watch2[cl] : (-watch2[cl] + dish_count);
    next2[cl] = head[p2];
    head[p2] = cl;

    return 1;
}

void backtrack(int bjl) {
    while (trh > 0) {
        int lit = trail[trh-1];
        int var = abs(lit);

        if (level[var] <= bjl) break;

        val[var] = 0; 
        reason[var] = 0;
        level[var] = -1;

        if (heap_pos[var] == 0)
            vinsert(var);

        trh--;
    }

    trq = trh;
    dec_level = bjl;
}

void restore(int mem) {
    starts_count = mem;

    /* new limits */
    int last = starts[starts_count];
    while(literals[last] != 0) last++; 
    lit_count = last+1; 

    /* restore adlist */
    memset(head, 0, (dish_count*2+1) * sizeof(int));
    twl(watch1, watch2);
    madlist(head, next1, next2);

    /* clean state */
    memset(val, 0, (dish_count+1) * sizeof(int));
    memset(reason, 0, (dish_count+1) * sizeof(int));
    memset(level, -1, (dish_count+1) * sizeof(int));

    heap_size = dish_count;
    var_inc = 1.0;
    for (int i = 1; i <= dish_count; i++) {
        heap[i] = i;
        heap_pos[i] = i;
        activity[i] = 0.0;
    }

    trh = 0; 
    trq = 0;
    dec_level = 0;
}


/* vsids heap */
void hswap(int i, int j) {
    int swap; 

    swap = heap[i];
    heap[i] = heap[j];
    heap[j] = swap;

    heap_pos[heap[i]] = i; 
    heap_pos[heap[j]] = j;
}

void hup(int pos) {
    while (pos > 1) {
        int parent = pos / 2;
        if (activity[heap[pos]] > activity[heap[parent]]) {
            hswap(pos, parent);
            pos = parent;
        }
        else break;
    } 
}

void hdown(int pos) {
    while (2 * pos <= heap_size) {
        int son = 2*pos;

        if (son < heap_size &&
            activity[heap[son+1]] > activity[heap[son]]) {
            son++;
        }

        if (activity[heap[son]] > activity[heap[pos]]) {
            hswap(pos, son);
            pos = son;
        }
        else break;
    }
}

void vbump(int var) {
    activity[var] += var_inc;
    if (heap_pos[var] > 0)
        hup(heap_pos[var]);
} 

void vdecay() {
    var_inc *= var_decay;
    if (var_inc > rescale_threshold) {
        var_inc /= rescale_threshold;
        for (int i = 1; i <= dish_count; i++)
            activity[i] /= rescale_threshold;
    }
}

void vinsert(int var) {
    heap_size++;
    heap[heap_size] = var;
    heap_pos[var] = heap_size;

    hup(heap_size);
}

int vlazy() {
    while (heap_size > 0) {
        int var = heap[1];

        hswap(1, heap_size);
        heap_pos[var] = 0;
        heap_size--;
        hdown(1);

        if (val[var] == 0)
            return var;
    }

    return 0;
}