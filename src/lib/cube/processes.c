#include "processes.h"
#include "memory.h"
#include "vm.h"
#include "mempool.h"
#include "util.h"

#include <signal.h>
#include <errno.h>
#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

static bool writeProccess(int argCount, bool newLine) {
	if (argCount != 2) {
		runtimeError("write() takes 2 arguments (%d given)", argCount);
		return false;
	}

	Value data = pop();
	ObjProcess *process = AS_PROCESS(pop());

	if(process->closed)
	{
		runtimeError("Cannot write to a finished process.\n");
		return false;
	}
	
	int wrote = 0;
	if(IS_BYTES(data))
	{
		ObjBytes *bytes = AS_BYTES( data );
		wrote = writeFd(process->out, bytes->length, (char*)bytes->bytes);
	}
	else
	{
		ObjString *string = AS_STRING( toString(data) );
		wrote = writeFd(process->out, string->length, string->chars);
	}
	if (newLine)
		writeFd(process->out, 1, "\n");

	processAlive(process);
	push(NUMBER_VAL(wrote));
	return true;
}

static bool readProcess(int argCount) {
	if (argCount == 0 || argCount > 2) {
		runtimeError("read() takes 1 or 2 arguments (%d given)", argCount);
		return false;
	}

	int maxSize = MAX_STRING_INPUT;

	if(argCount > 1)
	{
		Value ms = pop();
		if(IS_NUMBER(ms))
		{
			maxSize = AS_NUMBER(ms);
		}
	}

	ObjProcess *process = AS_PROCESS(pop());
	if(process->closed)
	{
		runtimeError("Cannot read from a finished process.\n");
		return false;
	}
	
	char *buffer = NULL;
	int size = 0;
	int rd = maxSize - size;
	if(rd > 256)
		rd = 256;

	char buf[256];
	int rc = readFd(process->in, rd, buf);
	while(rc > 0 && size <= maxSize)
	{
		buffer = mp_realloc(buffer, size + rc + 1);
		memcpy(buffer + size, buf, rc);
		size += rc;
		rd = maxSize - size;
		if(rd > 256)
			rd = 256;
		rc = readFd(process->in, rd, buf);
	}

	processAlive(process);
	if(rc < 0 && size == 0)
	{
		mp_free(buffer);
		if(errno != EAGAIN)
		{
			runtimeError("Could not read the process.\n");
			return false;
		}
		push(NONE_VAL);
		return true;
	}

	Value ret;
	if(buffer == NULL)
		push(STRING_VAL(""));
	else
	{
		buffer[size] = '\0';
		push(STRING_VAL(buffer));
		mp_free(buffer);
	}
	return true;
}

static bool readProcessBytes(int argCount) {
	if (argCount == 0 || argCount > 2) {
		runtimeError("readBytes() takes 1 or 2 arguments (%d given)", argCount);
		return false;
	}	

	int maxSize = MAX_STRING_INPUT;
	if(argCount > 1)
	{
		Value ms = pop();
		if(IS_NUMBER(ms))
		{
			maxSize = AS_NUMBER(ms);
		}
	}

	ObjProcess *process = AS_PROCESS(pop());
	if(process->closed)
	{
		runtimeError("Cannot read from a finished process.\n");
		return false;
	}
	
	unsigned char *buffer = NULL;
	int size = 0;
	int rd = maxSize - size;
	if(rd > 256)
		rd = 256;

	char buf[256];
	int rc = readFd(process->in, rd, buf);
	while(rc > 0 && size <= maxSize)
	{
		buffer = mp_realloc(buffer, size + rc);
		memcpy(buffer + size, buf, rc);
		size += rc;
		rd = maxSize - size;
		if(rd > 256)
			rd = 256;
		rc = readFd(process->in, rd, buf);
	}

	processAlive(process);
	if(rc < 0 && size == 0)
	{
		mp_free(buffer);
		if(errno != EAGAIN)
		{
			runtimeError("Could not read the process.\n");
			return false;
		}
		push(NONE_VAL);
		return true;
	}

	Value ret;
	if(buffer == NULL)
		push(BYTES_VAL("", 0));
	else
	{
		push(BYTES_VAL(buffer, size));
		mp_free(buffer);
	}
	return true;
}

static bool readLineProcess(int argCount) {
	if (argCount == 0 || argCount > 2) {
		runtimeError("read() takes 1 argument (%d given)", argCount);
		return false;
	}	
	int maxSize = MAX_STRING_INPUT;

	ObjProcess *process = AS_PROCESS(pop());
	if(process->closed)
	{
		runtimeError("Cannot read from a finished process.\n");
		return false;
	}

	int bufsize = 256;
	char *buffer = mp_realloc(buffer, bufsize);
	int size = 0;

	char buf[1];
	int rc = readFd(process->in, 1, buf);
	while(rc > 0 && size <= maxSize)
	{
		if(buf[0] == '\n')
			break;
		buffer[size] = buf[0];
		size += rc;
		if(size == bufsize)
		{
			bufsize += 256;
			buffer = mp_realloc(buffer, bufsize);
		}
		rc = readFd(process->in, 1, buf);
	}

	processAlive(process);
	if(rc < 0)
	{
		mp_free(buffer);
		if(errno != EAGAIN)
		{
			runtimeError("Could not read the process.\n");
			return false;
		}
		push(NONE_VAL);
		return true;
	}

	Value ret;
	if(buffer == NULL)
		push(STRING_VAL(""));
	else
	{
		buffer[size] = '\0';
		push(STRING_VAL(buffer));
		mp_free(buffer);
	}
	return true;
}

static bool closeProcess(int argCount) {
	if (argCount != 1) {
		runtimeError("close() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjProcess *process = AS_PROCESS(pop());

	if(!process->closed)
	{
		#ifdef _WIN32

        #else
        close(process->in);
        close(process->out);
        #endif 
		process->closed = true;
	}

	push(TRUE_VAL);
	return true;
}

static bool waitProcess(int argCount) {
	if (argCount != 1) {
		runtimeError("wait() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjProcess *process = AS_PROCESS(pop());

	if(process->running)
	{
		int status = -1;
		#ifdef _WIN32
		#else
		waitpid(process->pid, &status, 0);
		status = WEXITSTATUS(status);
		#endif
		process->status = status;
		process->running = false;
	}

	push(NUMBER_VAL(process->status));
	return true;
}

static bool statusProcess(int argCount) {
	if (argCount != 1) {
		runtimeError("status() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjProcess *process = AS_PROCESS(pop());
	processAlive(process);
	push(NUMBER_VAL(process->status));
	return true;
}

static bool runningProcess(int argCount) {
	if (argCount != 1) {
		runtimeError("running() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjProcess *process = AS_PROCESS(pop());
	processAlive(process);
	push(BOOL_VAL(process->running));
	return true;
}

static bool blockProcess(int argCount) {
	if (argCount == 0 || argCount > 2) {
		runtimeError("block() takes 1 or 2 arguments (%d given)", argCount);
		return false;
	}
	
	ObjProcess *process;
	if(argCount == 1)
	{
		process = AS_PROCESS(pop());
		bool nonblocking = false;
		#ifdef _WIN32
		#else
		nonblocking = (fcntl(process->in, F_GETFL) & O_NONBLOCK) != 0;
		#endif
		push(BOOL_VAL(!nonblocking));
		return true;
	}

	bool enable = AS_BOOL(toBool(pop()));

	process = AS_PROCESS(pop());

	if(enable)
	{
		#ifdef _WIN32
		#else
		fcntl(process->in,  F_SETFL, fcntl(process->in,  F_GETFL) & ~O_NONBLOCK);
		// fcntl(process->out, F_SETFL, fcntl(process->out, F_GETFL) & ~O_NONBLOCK);
		#endif
	}
	else
	{
		#ifdef _WIN32
		#else
		fcntl(process->in,  F_SETFL, fcntl(process->in,  F_GETFL) | O_NONBLOCK);
		// fcntl(process->out, F_SETFL, fcntl(process->out, F_GETFL) | O_NONBLOCK);
		#endif
	}
	
	processAlive(process);
	push(BOOL_VAL(enable));
	return true;
}

static bool killProcess(int argCount) {
	if (argCount != 1) {
		runtimeError("kill() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjProcess *process = AS_PROCESS(pop());

	if(process->running)
	{
		int status = -1;
		#ifdef _WIN32
		#else
		kill(process->pid, SIGKILL);
		waitpid(process->pid, &status, 0);
		status = WEXITSTATUS(status);
		#endif
		process->status = status;
		process->running = false;
	}

	push(NUMBER_VAL(process->status));
	return true;
}

bool processesMethods(char *method, int argCount) {
	if (strcmp(method, "write") == 0)
		return writeProccess(argCount, false);
	else if (strcmp(method, "writeLine") == 0)
		return writeProccess(argCount, true);
	else if (strcmp(method, "read") == 0)
		return readProcess(argCount);
	else if (strcmp(method, "readLine") == 0)
		return readLineProcess(argCount);
	else if (strcmp(method, "readBytes") == 0)
		return readProcessBytes(argCount);
	else if (strcmp(method, "wait") == 0)
		return waitProcess(argCount);
	else if (strcmp(method, "status") == 0)
		return statusProcess(argCount);
	else if (strcmp(method, "running") == 0)
		return runningProcess(argCount);
	else if (strcmp(method, "block") == 0)
		return blockProcess(argCount);
	else if (strcmp(method, "close") == 0)
		return closeProcess(argCount);
	else if (strcmp(method, "kill") == 0)
		return killProcess(argCount);
	
	runtimeError("Process has no method %s()", method);
	return false;
}