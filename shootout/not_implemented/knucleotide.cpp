// The Computer Language Benchmarks Game
// http://shootout.alioth.debian.org/

// contributed by James McIlree
// Thanks to The Anh Tran, some code & ideas borrowed from his C++ contribution

#include <algorithm>
#include <vector>
#include <iostream>
#include <sstream>

#include <string.h>

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/hash_policy.hpp>

/*
 * Foward decls
 */

class WorkItem;
class InputData;

/*
 * Constants...
 */

const char* fragments[] = { "*","**","ggt","ggta","ggtatt", "ggtattttaatt", "ggtattttaatttatagt" };
const size_t BUFFER_SIZE = (30 * 1024 * 1024);
const size_t BUFFER_BYTES_OVERLAP = 32;

/*
 * Global state (Ewww)
 */

WorkItem* workList = NULL;
volatile int moreWorkIsPossible = 1;

/*
 * Classes
 */

template <int KEY_SIZE>
struct Key {
    uint32_t hash;
    char key[KEY_SIZE];

    Key() : hash(0) {}
    Key(char const* str) {
        hash = 0;
        for (int i = 0; i < KEY_SIZE; ++i) {
            key[i] = str[i];
            hash = (hash * 183) + key[i];
        }
    }

    uint32_t operator() (const Key& k) const { return k.hash; }

    // specialized for each size
    bool operator() (const Key& key1, const Key& key2) const { return false; }
};

template<>
bool Key<1>::operator() (const Key& key1, const Key& key2) const {
    return key1.key[0] == key2.key[0];
}

template<>
bool Key<2>::operator() (const Key& key1, const Key& key2) const {
    return *((uint16_t*)key1.key) == *((uint16_t*)key2.key);
}

template<>
bool Key<3>::operator() (const Key& key1, const Key& key2) const {
    if ((*((uint16_t*)key1.key) == *((uint16_t*)key2.key)) &&
        key1.key[2] == key2.key[2])
        return true;

    return false;
}

template<>
bool Key<4>::operator() (const Key& key1, const Key& key2) const {
    return *((uint32_t*)key1.key) == *((uint32_t*)key2.key);
}

template<>
bool Key<6>::operator() (const Key& key1, const Key& key2) const {
    if ((*((uint32_t*)key1.key) == *((uint32_t*)key2.key)) &&
        (((uint16_t*)key1.key)[2] == ((uint16_t*)key2.key)[2]))
        return true;

    return false;
}

template<>
bool Key<12>::operator() (const Key& key1, const Key& key2) const {
    uint32_t* k1 = (uint32_t*)key1.key;
    uint32_t* k2 = (uint32_t*)key2.key;
    if (k1[0] == k2[0] &&
        k1[1] == k2[1] &&
        k1[2] == k2[2])
        return true;

    return false;
}

template<>
bool Key<18>::operator() (const Key& key1, const Key& key2) const {
    uint64_t* k1 = (uint64_t*)key1.key;
    uint64_t* k2 = (uint64_t*)key2.key;
    if (k1[0] == k2[0] &&
        k1[1] == k2[1] &&
        (((uint16_t*)key1.key)[8] == ((uint16_t*)key2.key)[8]))
        return true;

    return false;
}

template <int KEY_SIZE>
class HashTable : public __gnu_pbds::cc_hash_table < Key<KEY_SIZE>, uint32_t, Key<KEY_SIZE>, Key<KEY_SIZE> > {};

class WorkerThread {
public:
    HashTable<1> hash1;
    HashTable<2> hash2;
    HashTable<3> hash3;
    HashTable<4> hash4;
    HashTable<6> hash6;
    HashTable<12> hash12;
    HashTable<18> hash18;

    pthread_t pthread;

    // Silly hack for printing
    void* hashByLength(int length) {
        switch (length) {
            case 1: return &hash1;
            case 2: return &hash2;
            case 3: return &hash3;
            case 4: return &hash4;
            case 6: return &hash6;
            case 12: return &hash12;
            case 18: return &hash18;
            default: exit(667);
        }
    }
};

class WorkItem {
  public:
    int first;
    int last;
    int length;
    InputData* data;
    WorkItem* next;

    WorkItem(int f, int l, int len, InputData* d, WorkItem* n) : first(f), last(l), length(len), data(d), next(n) {}
};

class InputData {
public:
    size_t capacity;
    size_t length;
    size_t logicalIndex;
    char overlapBytes[BUFFER_BYTES_OVERLAP];
    char buffer[BUFFER_SIZE+1];

    InputData() : capacity(BUFFER_SIZE), length(0), logicalIndex(0) {}
    InputData(InputData& previous) : capacity(BUFFER_SIZE), length(0), logicalIndex(previous.logicalIndex + previous.length) {
        memcpy(overlapBytes, &previous.buffer[previous.length - BUFFER_BYTES_OVERLAP], BUFFER_BYTES_OVERLAP);
    }

    void addToWorkList() {
        WorkItem* head = NULL;
        int i,j;

        for (i=0; i<sizeof(fragments) / sizeof(const char*); i++) {
            size_t fragmentLength = strlen(fragments[i]);
            for (j=0; j<fragmentLength; j++) {
                int firstIndex = (logicalIndex == 0) ?  j : -((logicalIndex - j) % fragmentLength);
                int lastIndex = length - fragmentLength + 1;
                WorkItem* item = new WorkItem(firstIndex, lastIndex, fragmentLength, this, head);
                head = item;
            }
        }

        WorkItem* last = head;
        while (last->next)
            last = last->next;

        do {
            last->next = workList;
        } while (!__sync_bool_compare_and_swap(&workList, last->next, head));
    }
};

template <int KEY_SIZE>
void calculateFrequencies(InputData* data, HashTable<KEY_SIZE>& map, int firstIndex, int lastIndex) {
    int index;
    for (index=firstIndex; index<lastIndex; index+=KEY_SIZE) {
        Key<KEY_SIZE> k(&data->buffer[index]);
        map[k]++;
    }
}

void* doWork(WorkerThread* thread) {
    WorkItem* item;
    do {
        if (item = workList) {
            if (__sync_bool_compare_and_swap(&workList, item, item->next)) {
                switch (item->length) {
                    case 1:
                        calculateFrequencies(item->data, thread->hash1, item->first, item->last);
                        break;
                    case 2:
                        calculateFrequencies(item->data, thread->hash2, item->first, item->last);
                        break;
                    case 3:
                        calculateFrequencies(item->data, thread->hash3, item->first, item->last);
                        break;
                    case 4:
                        calculateFrequencies(item->data, thread->hash4, item->first, item->last);
                        break;
                    case 6:
                        calculateFrequencies(item->data, thread->hash6, item->first, item->last);
                        break;
                    case 12:
                        calculateFrequencies(item->data, thread->hash12, item->first, item->last);
                        break;
                    case 18:
                        calculateFrequencies(item->data, thread->hash18, item->first, item->last);
                        break;
                    default: exit(666);
                        break;
                }
                delete item;
            }
        } else {
            if (moreWorkIsPossible)
                usleep(1000);
        }
    } while (workList || moreWorkIsPossible);

    return NULL;
}

template<class T>
struct greater_second : std::binary_function<T,T,bool> {
    inline bool operator()(const T& lhs, const T& rhs) {
        return lhs.second > rhs.second;
    }
};

template <int KEY_SIZE>
void merge_table(HashTable<KEY_SIZE>& dest, HashTable<KEY_SIZE>& src) {
    for (typename HashTable<KEY_SIZE>::iterator it = src.begin(); it != src.end(); ++it) {
        dest[(*it).first] += (*it).second;
    }
}

template <int KEY_SIZE>
void printFreq(WorkerThread* threads, int cpus, float totalCount) {
    HashTable<KEY_SIZE> sum;
    for (int i=0; i<cpus; i++) {
        merge_table(sum, *((HashTable<KEY_SIZE>*)threads[i].hashByLength(KEY_SIZE)));
    }

    typedef std::pair< Key<KEY_SIZE>, uint32_t > hash_pair_t;
    std::vector<hash_pair_t> list(sum.begin(), sum.end());
    std::sort(list.begin(), list.end(), greater_second<hash_pair_t>());

    for (typename std::vector<hash_pair_t>::iterator it = list.begin(); it < list.end(); ++it) {
        char key[KEY_SIZE+1];
        for (int i=0; i<KEY_SIZE; i++) key[i] = toupper((*it).first.key[i]);
        key[KEY_SIZE] = 0;
        printf("%s %.3f\n", key, (float)((*it).second) * 100.0f / totalCount);
    }
    printf("\n");

}

template <int KEY_SIZE>
void printCount(WorkerThread* threads, int cpus, const char* fragment) {
    Key<KEY_SIZE> k(fragment);
    int count = 0;
    for (int i=0; i<cpus; i++) {
        count += (*(HashTable<KEY_SIZE>*)threads[i].hashByLength(KEY_SIZE))[k];
    }

    char key[KEY_SIZE+1];
    for (int i=0; i<KEY_SIZE; i++) key[i] = toupper(fragment[i]);
    key[KEY_SIZE] = 0;
    printf("%d\t%s\n", count, key);
}

int numCPUS() {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    sched_getaffinity(0, sizeof(cs), &cs);

    int count = 0;
    for (int i = 0; i < 16; ++i) {
        if (CPU_ISSET(i, &cs))
            ++count;
    }
    return count;
}

int main() {
    InputData* data = new InputData();

    while (fgets(data->buffer, data->capacity, stdin))
        if (strncmp(data->buffer, ">THREE", 6) == 0)
            break;

    int cpus = numCPUS();
    WorkerThread threads[cpus];
    for (int i = 0; i<cpus-1; i++) {
        pthread_create(&(threads[i].pthread), NULL, (void *(*)(void *))doWork, &threads[i]);
    }

    size_t totalBytesRead = 0;
    while (fgets(&data->buffer[data->length], (data->capacity + 1) - data->length, stdin)) {
        data->length += strlen(&data->buffer[data->length]);
        if (data->buffer[data->length-1] == '\n') {
            data->buffer[--(data->length)] == 0;
        }

        if (data->length == data->capacity) {
            data->addToWorkList();
            totalBytesRead += data->length;
            data = new InputData(*data);
        }
    }

    if (data->length) {
        totalBytesRead += data->length;
        data->addToWorkList();
    }

    moreWorkIsPossible = 0;
    doWork(&threads[cpus-1]);
    for (int i=0; i<cpus-1; i++) {
        pthread_join(threads[i].pthread, NULL);
    }

    printFreq<1>(threads, cpus, (float)totalBytesRead);
    printFreq<2>(threads, cpus, (float)(totalBytesRead - 1));

    printCount<3>(threads, cpus, fragments[2]);
    printCount<4>(threads, cpus, fragments[3]);
    printCount<6>(threads, cpus, fragments[4]);
    printCount<12>(threads, cpus, fragments[5]);
    printCount<18>(threads, cpus, fragments[6]);

    return 0;
}
