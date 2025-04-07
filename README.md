# Bloom_Filters
check username availability for a new gaming platform
a probabilistic data structure (Bloom filter) to check username availability in a gaming platform with:
- 1000-bit filter size
-	2 hash functions (DJB2 and SDBM)
-	10,000 preloaded usernames
-	Edge case handling

Solution Approach:
-	Two-layer verification (Bloom filter → linear search fallback)
-	Cached database with CSV persistence
-	Comprehensive input validation
