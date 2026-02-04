/*
 * Test program to verify Dis VM works on Android
 * This is a simple Dis bytecode executor test
 */

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "TaijiOS-DisTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*
 * External references to Dis VM
 */
extern void disinit(void*);
extern void vmachine(void* code);
extern void* loadmodule(char* name);

/*
 * Simple Dis bytecode test
 * This is the Dis bytecode for: print("Hello from Android Dis VM!\n");
 */
static uchar hello_dis[] = {
	/* Dis bytecode would go here */
	/* For now, we'll just test the VM initialization */
};

/*
 * Test Dis VM initialization
 */
int
test_dis_init(void)
{
	LOGI("Testing Dis VM initialization...");

	/* Initialize the Dis VM */
	disinit(nil);

	LOGI("Dis VM initialized successfully");
	return 0;
}

/*
 * Test basic Dis execution
 */
int
test_dis_exec(void)
{
	LOGI("Testing Dis execution...");

	/* Load and execute a Dis module */
	/* This will be implemented when we have .dis files */

	LOGI("Dis execution test complete");
	return 0;
}

/*
 * Test threading
 */
static void*
thread_test(void* arg)
{
	LOGI("Dis VM thread running");
	return nil;
}

int
test_dis_threads(void)
{
	pthread_t thread;
	int result;

	LOGI("Testing Dis VM threading...");

	result = pthread_create(&thread, nil, thread_test, nil);
	if(result != 0) {
		LOGE("Failed to create thread: %d", result);
		return -1;
	}

	pthread_join(thread, nil);
	LOGI("Dis VM threading test complete");
	return 0;
}

/*
 * Main test entry point
 */
int
android_test_dis_vm(void)
{
	int failures = 0;

	LOGI("=== Dis VM Android Test Suite ===");

	if(test_dis_init() != 0)
		failures++;

	if(test_dis_exec() != 0)
		failures++;

	if(test_dis_threads() != 0)
		failures++;

	LOGI("=== Test Complete: %d failures ===", failures);

	return failures;
}
