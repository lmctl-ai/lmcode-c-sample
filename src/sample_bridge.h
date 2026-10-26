#ifndef LMCODE_C_H
#define LMCODE_C_H

/* C ABI for lmcode-cpp's header-only DSL VM. Handles are opaque. Registry and
 * context handles must outlive programs/runs that use them. Callback argument
 * buffers are borrowed only during the callback; callback-returned strings and
 * type names must remain valid until the callback returns because the adapter
 * copies them at that boundary. Never return stack-local buffers. Object
 * pointers are borrowed for the run. */
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sample_registry sample_registry;
typedef struct sample_context sample_context;
typedef struct sample_program sample_program;

typedef enum sample_value_kind {
  LMCODE_VOID = 0,
  LMCODE_INT = 1,
  LMCODE_FLOAT = 2,
  LMCODE_STRING = 3,
  LMCODE_BOOL = 4,
  LMCODE_OBJECT = 5
} sample_value_kind;
typedef struct sample_value {
  sample_value_kind kind;
  int64_t i;
  double f;
  int b;
  const char *data;
  size_t length;
  void *object;
  const char *type;
  size_t type_length;
} sample_value;

typedef struct sample_error {
  const char *code;
  const char *message;
  uint32_t line;
  uint16_t column;
  int has_location;
  int has_column;
} sample_error;

/* Return zero on success. On failure, out_error is filled with borrowed text
 * valid until the next call on the same handle (signature text is valid until
 * the next signature query or registry mutation). Callbacks return zero and
 * fill
 * result, or nonzero and fill error (message/code are copied immediately). */
typedef int (*sample_function_callback)(void *user, const sample_value *args,
                                        size_t count, sample_value *result,
                                        sample_error *error);
typedef int (*sample_method_callback)(void *user, void *self,
                                      const sample_value *args, size_t count,
                                      sample_value *result,
                                      sample_error *error);
/* Testing print receives every byte written by print(), including newlines and
 * embedded NUL bytes. The data pointer is borrowed only for this call. */
typedef void (*sample_print_callback)(void *user, const char *data,
                                      size_t length);

sample_registry *sample_registry_create(void);
sample_registry *sample_registry_create_without_std(void);
void sample_registry_destroy(sample_registry *registry);
int sample_registry_function(sample_registry *, const char *name,
                             const char *const *params, size_t count,
                             const char *returns, sample_function_callback,
                             void *user, sample_error *error);
int sample_registry_method(sample_registry *, const char *type,
                           const char *name, const char *const *params,
                           size_t count, const char *returns,
                           sample_method_callback, void *user,
                           sample_error *error);
int sample_registry_context(sample_registry *, const char *name,
                            const char *type, sample_error *error);
size_t sample_registry_signature_count(const sample_registry *);
const char *sample_registry_signature(const sample_registry *, size_t index);
int sample_registry_register_testing(sample_registry *, sample_print_callback,
                                     void *user, sample_error *error);
int sample_registry_call_function(sample_registry *, const char *signature,
                                  const sample_value *args, size_t count,
                                  sample_value *result, sample_error *error);
int sample_registry_call_method(sample_registry *, const char *signature,
                                void *self, const sample_value *args,
                                size_t count, sample_value *result,
                                sample_error *error);

sample_context *sample_context_create(void);
void sample_context_destroy(sample_context *);
int sample_context_set(sample_context *, const char *name, void *object,
                       sample_error *error);
int sample_context_set_file(sample_context *, const char *file,
                            sample_error *error);
int sample_context_set_dry(sample_context *, int dry, sample_error *error);
int sample_context_set_step_budget(sample_context *, uint64_t budget,
                                   sample_error *error);

int sample_program_load(const void *bytecode, size_t bytecode_length,
                        const void *schema, size_t schema_length,
                        const sample_registry *, sample_program **out,
                        sample_error *error);
int sample_program_run(const sample_program *, sample_context *,
                       sample_error *error);
void sample_program_destroy(sample_program *);

/* Standard one-line result helpers for C hosts. Return the process exit code.
 */
int sample_report_ok_json(const char *json);
int sample_report_error(const char *phase, const sample_error *error);

#ifdef __cplusplus
}
#endif
#endif
