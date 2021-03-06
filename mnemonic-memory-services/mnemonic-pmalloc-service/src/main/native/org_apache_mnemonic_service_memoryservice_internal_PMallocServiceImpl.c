/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl.h"

static PMPool *g_pmpool_arr = NULL;
static size_t g_pmpool_count = 0;

static pthread_rwlock_t g_pmp_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/******************************************************************************
 ** JNI implementations
 *****************************************************************************/

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nallocate(JNIEnv* env,
    jobject this, jlong id, jlong size, jboolean initzero) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  void* nativebuf = prealloc(pool, NULL, size, initzero);
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return addr_to_java(nativebuf);
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nreallocate(JNIEnv* env,
    jobject this, jlong id, jlong addr, jlong size, jboolean initzero) {
  PMPool *pool;
  void* nativebuf = NULL;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  void* p = addr_from_java(addr);
  nativebuf = prealloc(pool, p, size, initzero);
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return addr_to_java(nativebuf);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nfree(
    JNIEnv* env,
    jobject this, jlong id,
    jlong addr) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  void* nativebuf = addr_from_java(addr);
  pfree(pool, nativebuf);
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nsync(
    JNIEnv* env,
    jobject this, jlong id, jlong addr, jlong len, jboolean autodetect) {
  PMPool *pool;
  void *nativebuf;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  void *p = addr_from_java(addr);
  if (autodetect) {
    if (NULL != p) {
      nativebuf = p - PMBHSZ;
      pmsync(pool->pmp, nativebuf, ((PMBHeader *) nativebuf)->size);
    } else {
      pmsync(pool->pmp, b_addr(pool->pmp), pool->capacity);
    }
  } else {
    if (NULL != p && len > 0L) {
      pmsync(pool->pmp, p, len);
    }
  }
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_npersist(
    JNIEnv* env,
    jobject this, jlong id, jlong addr, jlong len, jboolean autodetect) {
  PMPool *pool;
  void *nativebuf;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  void *p = addr_from_java(addr);
  if (autodetect) {
    if (NULL != p) {
      nativebuf = p - PMBHSZ;
      pmsync(pool->pmp, nativebuf, ((PMBHeader *) nativebuf)->size);
    } else {
      pmsync(pool->pmp, b_addr(pool->pmp), pool->capacity);
    }
  } else {
    if (NULL != p && len > 0L) {
      pmsync(pool->pmp, p, len);
    }
  }
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nflush(
    JNIEnv* env,
    jobject this, jlong id, jlong addr, jlong len, jboolean autodetect) {
  PMPool *pool;
  void *nativebuf;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  void *p = addr_from_java(addr);
  if (autodetect) {
    if (NULL != p) {
      nativebuf = p - PMBHSZ;
      pmsync(pool->pmp, nativebuf, ((PMBHeader *) nativebuf)->size);
    } else {
      pmsync(pool->pmp, b_addr(pool->pmp), pool->capacity);
    }
  } else {
    if (NULL != p && len > 0L) {
      pmsync(pool->pmp, p, len);
    }
  }
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ndrain(
    JNIEnv* env,
    jobject this, jlong id)
{
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ncapacity(
    JNIEnv* env,
    jobject this, jlong id) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  jlong ret = pool->capacity;
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
jobject JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ncreateByteBuffer(
    JNIEnv *env, jobject this, jlong id, jlong size) {
  PMPool *pool;
  jobject ret = NULL;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  void* nativebuf = prealloc(pool, NULL, size, 0);
  if (NULL != nativebuf) {
    ret = (*env)->NewDirectByteBuffer(env, nativebuf, size);
  }
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
jobject JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nretrieveByteBuffer(
    JNIEnv *env, jobject this, jlong id, jlong addr) {
  jobject ret = NULL;
  void* p = addr_from_java(addr);
  if (NULL != p) {
    void* nativebuf = p - PMBHSZ;
    ret = (*env)->NewDirectByteBuffer(env, p, ((PMBHeader *) nativebuf)->size - PMBHSZ);
  }
  return ret;
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nretrieveSize(JNIEnv *env,
    jobject this, jlong id, jlong addr) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  void* p = addr_from_java(addr);
  jlong ret = psize(pool, p);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ngetByteBufferHandler(
    JNIEnv *env, jobject this, jlong id, jobject bytebuf) {
//	fprintf(stderr, "ngetByteBufferAddress Get Called %X, %X\n", env, bytebuf);
  jlong ret = 0L;
  if (NULL != bytebuf) {
    void* nativebuf = (*env)->GetDirectBufferAddress(env, bytebuf);
//    	fprintf(stderr, "ngetByteBufferAddress Get Native addr %X\n", nativebuf);
    ret = addr_to_java(nativebuf);
  }
//    fprintf(stderr, "ngetByteBufferAddress returned addr %016lx\n", ret);
  return ret;
}

JNIEXPORT
jobject JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nresizeByteBuffer(
    JNIEnv *env, jobject this, jlong id, jobject bytebuf, jlong size) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  jobject ret = NULL;
  if (NULL != bytebuf) {
    void* nativebuf = (*env)->GetDirectBufferAddress(env, bytebuf);
    if (nativebuf != NULL) {
      nativebuf = prealloc(pool, nativebuf, size, 0);
      if (NULL != nativebuf) {
        ret = (*env)->NewDirectByteBuffer(env, nativebuf, size);
      }
    }
  }
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ndestroyByteBuffer(
    JNIEnv *env, jobject this, jlong id, jobject bytebuf) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  if (NULL != bytebuf) {
    void* nativebuf = (*env)->GetDirectBufferAddress(env, bytebuf);
    pfree(pool, nativebuf);
  }
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nsetHandler(
    JNIEnv *env, jobject this, jlong id, jlong key, jlong value) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  if (key < PMALLOC_KEYS && key >= 0) {
    pmalloc_setkey(pool->pmp, key, (void*)value);
  }
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ngetHandler(JNIEnv *env,
    jobject this, jlong id, jlong key) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  jlong ret = (key < PMALLOC_KEYS && key >= 0) ? (long) pmalloc_getkey(pool->pmp, key) : 0;
  pthread_mutex_unlock(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nhandlerCapacity(
    JNIEnv *env, jobject this) {
  return PMALLOC_KEYS;
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ngetBaseAddress(JNIEnv *env,
    jobject this, jlong id) {
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  jlong ret = (long) b_addr(pool->pmp);
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
jlong JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_ninit(JNIEnv *env,
    jclass this, jlong capacity, jstring pathname, jboolean isnew) {
  PMPool *pool;
  size_t ret = -1;
  void *md = NULL;
  pthread_rwlock_wrlock(&g_pmp_rwlock);
  const char* mpathname = (*env)->GetStringUTFChars(env, pathname, NULL);
  if (NULL == mpathname) {
    pthread_rwlock_unlock(&g_pmp_rwlock);
    throw(env, "Big memory path not specified!");
  }
  if ((md = pmopen(mpathname, NULL,
                   PMALLOC_MIN_POOL_SIZE > capacity ? PMALLOC_MIN_POOL_SIZE : capacity)) == NULL) {
    pthread_rwlock_unlock(&g_pmp_rwlock);
    throw(env, "Big memory init failure!");
  }
  (*env)->ReleaseStringUTFChars(env, pathname, mpathname);
  g_pmpool_arr = realloc(g_pmpool_arr, (g_pmpool_count + 1) * sizeof(PMPool));
  if (NULL != g_pmpool_arr) {
    pool = g_pmpool_arr + g_pmpool_count;
    pool->pmp = md;
    pool->capacity = pmcapacity(pool->pmp);
    pthread_mutex_init(&pool->mutex, NULL);
    ret = g_pmpool_count;
    ++g_pmpool_count;
  } else {
    pthread_rwlock_unlock(&g_pmp_rwlock);
    throw(env, "Big memory init Out of memory!");
  }
  pthread_rwlock_unlock(&g_pmp_rwlock);
  return ret;
}

JNIEXPORT
void JNICALL Java_org_apache_mnemonic_service_memoryservice_internal_PMallocServiceImpl_nclose
(JNIEnv *env, jobject this, jlong id)
{
  PMPool *pool;
  pthread_rwlock_rdlock(&g_pmp_rwlock);
  pool = g_pmpool_arr + id;
  pthread_mutex_lock(&pool->mutex);
  if (NULL != pool->pmp) {
    pmclose(pool->pmp);
    pool->pmp = NULL;
    pool->capacity = 0;
  }
  pthread_mutex_unlock(&pool->mutex);
  pthread_mutex_destroy(&pool->mutex);
  pthread_rwlock_unlock(&g_pmp_rwlock);
}

__attribute__((destructor)) void fini(void) {
  int i;
  PMPool *pool;
  if (NULL != g_pmpool_arr) {
    for (i = 0; i < g_pmpool_count; ++i) {
      pool = g_pmpool_arr + i;
      if (NULL != pool->pmp) {
        pmclose(pool->pmp);
        pool->pmp = NULL;
        pool->capacity = 0;
        pthread_mutex_destroy(&pool->mutex);
      }
    }
    free(g_pmpool_arr);
    g_pmpool_arr = NULL;
    g_pmpool_count = 0;
  }
  pthread_rwlock_destroy(&g_pmp_rwlock);
}
