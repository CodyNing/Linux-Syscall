#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define ANCESTOR_NAME_LEN 16
#define TEST_SIZE_ONE 10
#define TEST_SIZE_TWO 11
#define TEST_SIZE_THREE 3

// Syscall numbers
#if _LP64 == 1
// 64 bit
#define SYSCALL_CS300_TEST 548
#define SYSCALL_ARRAY_STATS 549
#define SYSCALL_PROCESS_ANCESTOR 550
#else
// 32 bit
#define SYSCALL_CS300_TEST 439
#define SYSCALL_ARRAY_STATS 440
#define SYSCALL_PROCESS_ANCESTOR 441
#endif

// Macro for custom testing; does exit(1) on failure.
#define CHECK(condition)                                                                          \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            printf("ERROR: %s (@%d): failed condition \"%s\"\n", __func__, __LINE__, #condition); \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)

struct array_stats_s
{
    long min;
    long max;
    long sum;
};

struct process_info
{
    long pid;                     /* Process ID */
    char name[ANCESTOR_NAME_LEN]; /* Program name of process */
    long state;                   /* Current process state */
    long uid;                     /* User ID of process owner */
    long nvcsw;                   /* # voluntary context switches */
    long nivcsw;                  /* # involuntary context switches */
    long num_children;            /* # children process has */
    long num_siblings;            /* # sibling process has */
};

static void s_test_array_stats()
{

    long arr1[TEST_SIZE_ONE] = {
        5, 4, 3, 2, 1, 0, 9, 8, 7, 6};
    long arr2[TEST_SIZE_TWO] = {
        LONG_MIN, 4, LONG_MAX, 2, 1, -1, 9, 8, 7, 6, 1000};
    long *bad_arr = NULL;
    //make sure we passed long
    //number literal is int and will be translated in different value in long when passed in syscall
    long result, size_one = TEST_SIZE_ONE, size_two = TEST_SIZE_TWO, size_neg = -1, size_zero = 0;

    struct array_stats_s stats;
    struct array_stats_s * p_bad_stats = NULL;

    //valid size 10
    result = syscall(SYSCALL_ARRAY_STATS, &stats, arr1, size_one);
    CHECK(result == 0);
    CHECK(stats.sum == 45);
    CHECK(stats.min == 0);
    CHECK(stats.max == 9);

    //valid size 1
    result = syscall(SYSCALL_ARRAY_STATS, &stats, arr2, size_two);
    CHECK(result == 0);
    CHECK(stats.sum == 1035);
    CHECK(stats.min == LONG_MIN);
    CHECK(stats.max == LONG_MAX);

    //valid size 11 array but passed size 10
    result = syscall(SYSCALL_ARRAY_STATS, &stats, arr2, size_one);
    CHECK(result == 0);
    CHECK(stats.sum == 35);
    CHECK(stats.min == LONG_MIN);
    CHECK(stats.max == LONG_MAX);

    //unexpected behavior: size 10 array but passed size 11
    //result = syscall(SYSCALL_ARRAY_STATS, &stats, arr, TEST_SIZE_TWO);
    //CHECK(result == -EFAULT);

    //error size 0
    result = syscall(SYSCALL_ARRAY_STATS, &stats, arr1, size_zero);
    CHECK(result == -1);
    CHECK(errno == EINVAL);

    //error size -1
    result = syscall(SYSCALL_ARRAY_STATS, &stats, arr1, size_neg);
    CHECK(result == -1);
    CHECK(errno == EINVAL);

    //error bad struct pointer
    result = syscall(SYSCALL_ARRAY_STATS, p_bad_stats, arr1, size_one);
    CHECK(result == -1);
    CHECK(errno == EFAULT);

    //error bad array pointer
    result = syscall(SYSCALL_ARRAY_STATS, &stats, bad_arr, size_one);
    CHECK(result == -1);
    CHECK(errno == EFAULT);

}

static void s_test_process_ancestor()
{

    struct process_info info_array[TEST_SIZE_THREE];
    struct process_info *p_bad_info_array = NULL;
    struct process_info p_info;
    long i = 0, num_filled = 0, result, size_three = TEST_SIZE_THREE, size_neg = -1, size_zero = 0;
    long *p_bad_num_filled = NULL;

    //valid
    result = syscall(SYSCALL_PROCESS_ANCESTOR, info_array, TEST_SIZE_THREE, &num_filled);
    CHECK(result == 0);
    CHECK(num_filled <= TEST_SIZE_THREE);
    printf("num filled %ld\n", num_filled);
    for (i = 0; i < num_filled; ++i)
    {
        p_info = info_array[i];
        printf("My PID %ld\n", p_info.pid);
        printf("My Name %s\n", p_info.name);
        printf("My state %ld\n", p_info.state);
        printf("My nvcsw %ld\n", p_info.nvcsw);
        printf("My nivcsw %ld\n", p_info.nivcsw);
        printf("My uid %ld\n", p_info.uid);
        printf("My num_children %ld\n", p_info.num_children);
        printf("My num_siblings %ld\n", p_info.num_siblings);
    }

    //error size 0
    result = syscall(SYSCALL_PROCESS_ANCESTOR, info_array, size_zero, &num_filled);
    CHECK(result == -1);
    CHECK(errno == EINVAL);

    //error size -1
    result = syscall(SYSCALL_PROCESS_ANCESTOR, info_array, size_neg, &num_filled);
    CHECK(result == -1);
    CHECK(errno == EINVAL);

    //error bad struct array pointer
    result = syscall(SYSCALL_PROCESS_ANCESTOR, p_bad_info_array, size_three, &num_filled);
    CHECK(result == -1);
    CHECK(errno == EFAULT);

    //error bad num filled pointer
    result = syscall(SYSCALL_PROCESS_ANCESTOR, info_array, size_three, p_bad_num_filled);
    CHECK(result == -1);
    CHECK(errno == EFAULT);
}

int main(int argc, char *argv[])
{
    s_test_array_stats();
    s_test_process_ancestor();

    printf("\n All test passed. \n");

    return 0;
}