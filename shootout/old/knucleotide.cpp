	41.52	134,648	1418	13.61	  67% 66% 100% 71%

// The Computer Language Benchmarks Game
// http://shootout.alioth.debian.org/
// Contributed by The Anh Tran

#include <omp.h>
#include <sched.h>

#include <cstdio>

#include <algorithm>
#include <vector>
#include <ext/hash_map>

#include <boost/foreach.hpp>
#define for_each BOOST_FOREACH

typedef unsigned int uint;

// Hashtable key, with key's size is equal to reading_frame_size
template <int size>
struct hash_key
{
   uint   hash_val;
   char   key[size +1];

   hash_key()
   {
      memset(this, 0, sizeof(*this));
   }

   hash_key(const char * str)
   {
      ReHash(str);
   }

   void ReHash(char const * str)
   {
      memcpy(key, str, size);
      key[size] = 0;

      hash_val = 0;
      for (int i = 0; i < size; i++)
         hash_val = (hash_val * 31) + key[i];
   }

   // equal_to<K>(Left, Right) comparison
   inline operator uint() const
   {
      return hash_val;
   }

   // g++
   // override hash<K>(Key &)
   inline uint operator() (const hash_key &k) const
   {
      return k.hash_val;
   }
};


template <int hash_len, typename INPUT, typename HTBL>
static inline
void calculate_frequency(INPUT const &input, HTBL& hash_table)
{
   char const* it = &(input[0]);
   char const* end = it + input.size() - hash_len +1;
   typename HTBL::key_type key;

   for (; it != end; it++)
   {
      key.ReHash(it);
      ++(hash_table[key]);
   }
}

template<typename T>
inline static
bool decrease_pred(const T &left, const T &right)
{
   return !(left.second < right.second);
}

template <int hash_len, typename INPUT, size_t out_len>
static
void write_frequencies(INPUT const &input,  char (&output)[out_len])
{
   typedef hash_key<hash_len> KEY;
   typedef __gnu_cxx::hash_map<KEY, uint, KEY > HTBL;

   // Build hash table
   HTBL hash_table;
   calculate_frequency<hash_len>(input, hash_table);

   typedef std::pair<KEY, uint> ELEMENT;
   typedef std::vector< ELEMENT > KTBL;

   // Copy result from hashtable to vector
   KTBL order_tbl(hash_table.begin(), hash_table.end());
   // Sort with descending frequency
   std::sort(order_tbl.begin(), order_tbl.end(), decrease_pred<ELEMENT> );

   size_t printedchar = 0;
   float totalchar = float(input.size() - hash_len +1);

   for_each (ELEMENT &i, order_tbl)
   {
      std::transform(i.first.key +0, i.first.key +hash_len, i.first.key +0, toupper);

      printedchar += sprintf(   output +printedchar, "%s %0.3f\n",
         i.first.key, float(i.second) * 100.0f / totalchar   );
   }

   memcpy(output + printedchar, "\n", 2);
}

// Build a hashtable, count all key with hash_len = reading_frame_size
// Then print a specific sequence's count
template <int hash_len, typename INPUT, size_t out_len>
static
void write_frequencies(INPUT const &input,  char (&output)[out_len], const char* specific)
{
   typedef hash_key<hash_len> KEY;
   typedef __gnu_cxx::hash_map<KEY, uint, KEY > HTBL;

   // build hash table
   HTBL hash_table;
   calculate_frequency<hash_len>(input, hash_table);

   // lookup specific frame
   KEY printkey(specific);
   uint count = hash_table[printkey];

   std::transform(printkey.key +0, printkey.key +hash_len, printkey.key +0, toupper);

   sprintf(output, "%d\t%s\n", count, printkey.key);
}

static
int GetThreadCount()
{
   cpu_set_t cs;
   CPU_ZERO(&cs);
   sched_getaffinity(0, sizeof(cs), &cs);

   int count = 0;
   for (int i = 0; i < 16; i++)
   {
      if (CPU_ISSET(i, &cs))
         count++;
   }
   return count;
}

int main()
{
   std::vector< char > input;
   input.reserve(256*1024*1024); // 256MB

   char buffer[128];
   while (fgets(buffer, sizeof(buffer), stdin))
   {
      if(strncmp(buffer, ">THREE", 6) == 0)
         break;
   }
   // rule: read line-by-line
   while (fgets(buffer, sizeof(buffer), stdin))
   {
      size_t sz = strlen(buffer);
      if (buffer[sz -1] == '\n')
         sz = sz -1;
      input.insert(input.end(), buffer, buffer + sz);
   }


   char output[7][256];
   #pragma omp parallel sections num_threads(GetThreadCount()) default(shared)
   {
      #pragma omp section
      write_frequencies<18>(input, output[6], "ggtattttaatttatagt" );
      #pragma omp section
      write_frequencies<12>(input, output[5], "ggtattttaatt" );
      #pragma omp section
      write_frequencies< 6>(input, output[4], "ggtatt" );
      #pragma omp section
      write_frequencies< 4>(input, output[3], "ggta" );
      #pragma omp section
      write_frequencies< 3>(input, output[2], "ggt" );
      #pragma omp section
      write_frequencies< 2>(input, output[1] );
      #pragma omp section
      write_frequencies< 1>(input, output[0] );
   }

   for ( int i = 0; i < 7; i++ )
      printf("%s", output[i]);
}


