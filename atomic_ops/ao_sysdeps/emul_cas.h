/*
 * Copyright (c) 2003 by Hewlett-Packard Company.  All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 */

/*
 * Ensure, if at all possible, that AO_compare_and_swap_full() is
 * available.  The emulation should be brute-force signal-safe, even
 * though it actually blocks.
 * Including this file will generate an error if AO_compare_and_swap_full()
 * cannot be made available.
 * This will be included from platform-specific atomic_ops files
 * id appropriate, and if AO_FORCE_CAS is defined.  It should not be
 * included directly, especially since it affects the implementation
 * of other atomic update primitives.
 * The implementation assumes that only AO_store_XXX and AO_test_and_set_XXX
 * variants are defined, and that AO_test_and_set_XXX is not used to
 * operate on compare_and_swap locations.
 */

#if !defined(ATOMIC_OPS_H)
#  error This file should not be included directly.
#endif

AO_T AO_compare_and_swap_emulation(volatile AO_T *addr, AO_T old,
				   AO_T new_val);

void AO_store_full_emulation(volatile AO_T *addr, AO_T val);

#define AO_compare_and_swap_full(addr, old, newval) \
	AO_compare_and_swap_emulation(addr, old, newval)
#define AO_HAVE_compare_and_swap_full

#undef AO_store
#undef AO_HAVE_store
#undef AO_store_write
#undef AO_HAVE_store_write
#undef AO_store_release
#undef AO_HAVE_store_release
#undef AO_store_full
#undef AO_HAVE_store_full
#define AO_store_full(addr, val) AO_store_full_emulation(addr, val)
#define AO_HAVE_store_full