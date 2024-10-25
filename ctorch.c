#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

/////////////////////////////////////////////////////////////////////

// Structure to hold an n-dimensional tensor with index mapping
typedef struct {
    int *shape;        // Array to hold the size of each dimension
    int dimensions;    // Number of dimensions
    double *data;      // Shared data array
    int total_size;    // Total number of elements in the tensor
    int *strides;      // Array to hold the stride of each dimension
    int *org_strides;  // Array to hold the stride of each dimension regardless of whether it is a shared transposed tensor or not
} Tensor;

// Function to calculate the total size of the tensor
int calculate_total_size(
    int *shape,
    int dimensions
) {
    int total_size = 1;
    for (int i = 0; i < dimensions; i++) {
        total_size *= shape[i];
    }
    return total_size;
}

// Function to calculate strides for a tensor based on its shape
void calculate_strides(
    int *shape,
    int dimensions,
    int *out_strides
) {
    out_strides[dimensions - 1] = 1;
    for (int i = dimensions - 2; i >= 0; i--) {
        out_strides[i] = out_strides[i + 1] * shape[i + 1];
    }
}

// Function to free the tensor's memory
void free_tensor(
    Tensor *tensor,
    bool free_data
) {
    free(tensor->shape);
    free(tensor->strides);
    free(tensor->org_strides);
    if (free_data) free(tensor->data);
    free(tensor);
}

// Function to initialize a tensor with a given shape
void create_tensor(
    int *shape,
    int dimensions,
    Tensor **out_tensor
) {
    int total_size = calculate_total_size(shape, dimensions);

    // Allocate memory and initialization
    *out_tensor = (Tensor *)malloc(sizeof(Tensor));
    (*out_tensor)->shape = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->strides = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->org_strides = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->data = (double *)malloc(total_size * sizeof(double));
    
    (*out_tensor)->dimensions = dimensions;
    (*out_tensor)->total_size = total_size;
    
    memcpy((*out_tensor)->shape, shape, dimensions * sizeof(int));
    calculate_strides(shape, dimensions, (*out_tensor)->strides);
    calculate_strides(shape, dimensions, (*out_tensor)->org_strides);
}

// Function to initialize a tensor with a given shape
void deep_copy_tensor(
    Tensor *tensor,
    Tensor **out_tensor
) {
    double *data = tensor->data;
    int *shape = tensor->shape;
    int dimensions = tensor->dimensions;
    int total_size = calculate_total_size(shape, dimensions);

    // Allocate memory and initialization
    *out_tensor = (Tensor *)malloc(sizeof(Tensor));
    (*out_tensor)->shape = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->strides = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->org_strides = (int *)malloc(dimensions * sizeof(int));
    (*out_tensor)->data = (double *)malloc(total_size * sizeof(double));

    (*out_tensor)->dimensions = dimensions;
    (*out_tensor)->total_size = total_size;
    
    memcpy((*out_tensor)->data, data, total_size * sizeof(double));
    memcpy((*out_tensor)->shape, shape, dimensions * sizeof(int));
    calculate_strides(shape, dimensions, (*out_tensor)->strides);
    calculate_strides(shape, dimensions, (*out_tensor)->org_strides);
}

void create_tensor_from_scalar(
    double value,
    Tensor **out_tensor
) {
    int tensor_shape[] = {1, 1};
    create_tensor(tensor_shape, 2, out_tensor);
    (*out_tensor)->data[0] = value;
}

void create_tensor_from_tensor(
    Tensor *tensor,
    int index,
    Tensor **out_tensor
) {
    int new_tensor_shape[] = {1, 1};
    create_tensor(new_tensor_shape, 2, out_tensor);
    free((*out_tensor)->data);
    (*out_tensor)->data = &tensor->data[index];
}

// Initialize all elements to default_vlaue
void init_tensor(
    Tensor *tensor,
    double default_vlaue
) {
    for (int i = 0; i < tensor->total_size; i++) {
        tensor->data[i] = default_vlaue;
    }
}

// Initialize all elements randomly between 0 and 1
void init_tensor_rand(
    Tensor *tensor
) {
    // Fill each element of the tensor with a random double between 0 and 1
    for (int i = 0; i < tensor->total_size; i++) {
        tensor->data[i] = (double)rand() / RAND_MAX;
    }
}

// Function to calculate the flattened index for the tensor from multi-dimensional indices
int get_flat_index(
    Tensor *tensor,
    int *indices
) {
    int flat_index = 0;

    // Iterate through each dimension
    for (int i = 0; i < tensor->dimensions; i++) {
        int index = indices[i];
        if (index >= tensor->shape[i] || index < 0) {
            fprintf(stderr, "Error: Index out of bounds for dimension %d.\n", i);
            exit(EXIT_FAILURE);
        }

        // Use the precomputed stride to calculate the flat index
        flat_index += index * tensor->strides[i];
    }

    return flat_index;
}

// Function to calculate the multi-dimensional indices from a flattened index
void get_multi_dimensional_index(
    Tensor *tensor,
    int flat_index,
    int *out_multi_dim_indices
) {
    // Ensure the flat index is within bounds
    if (flat_index < 0 || flat_index >= tensor->total_size) {
        fprintf(stderr, "Error: Flattened index out of bounds.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate the indices for each dimension using strides
    for (int i = 0; i < tensor->dimensions; i++) {
        // Determine the index for this dimension using the corresponding precomputed stride
        out_multi_dim_indices[i] = flat_index / tensor->org_strides[i];
        
        // Update the flat_index to the remainder for the next dimensions
        flat_index %= tensor->org_strides[i];
    }
}

// Function to get an element from the tensor using multi-dimensional indices
double get_element(
    Tensor *tensor,
    ...
) {
    va_list args;
    int *index_in_d = (int *)malloc(tensor->dimensions * sizeof(int));
    
    va_start(args, tensor);
    for (int i = 0; i < tensor->dimensions; i++) {
        int index = va_arg(args, int);
        index_in_d[i] = index;
    }
    va_end(args);

    int flat_index = get_flat_index(tensor, index_in_d);

    // Free the index_in_d array
    free(index_in_d);

    return tensor->data[flat_index];
}

// Function to set an element in the tensor using multi-dimensional indices
void set_element(
    Tensor *tensor,
    double value,
    ...
) {
    va_list args;
    int *index_in_d = (int *)malloc(tensor->dimensions * sizeof(int));

    va_start(args, value);
    for (int i = 0; i < tensor->dimensions; i++) {
        int index = va_arg(args, int);
        index_in_d[i] = index;
    }
    va_end(args);

    int flat_index = get_flat_index(tensor, index_in_d);

    tensor->data[flat_index] = value;

    // Free the index_in_d array
    free(index_in_d);
}

// Function to compare two tensors for equality
bool equal(
    Tensor *a,
    Tensor *b
) {
    // Check if the number of dimensions is the same
    if (a->dimensions != b->dimensions) {
        return false;
    }

    // Check if the shape of each dimension is the same
    for (int i = 0; i < a->dimensions; i++) {
        if (a->shape[i] != b->shape[i]) {
            return false;
        }
    }

    // Check if the data in each tensor is the same
    int *indices = (int *)malloc(a->dimensions * sizeof(int));
    for (int i = 0; i < a->total_size; i++) {
        // Get the multi-dimensional index for the current flat index
        // Multi-dim is the common index type among A and AT
        get_multi_dimensional_index(a, i, indices);
        int flat_index_a = get_flat_index(a, indices);
        int flat_index_b = get_flat_index(b, indices);

        // Compare the data values at the calculated flat indices
        if (a->data[flat_index_a] != b->data[flat_index_b]) {
            free(indices);
            return false;
        }
    }

    // Free allocated memory
    free(indices);

    // If all checks passed, the tensors are equal
    return true;
}

// Function to compare two tensors for equality except their data
bool equal_exclude_data(
    Tensor *a,
    Tensor *b
) {
    // Check if the number of dimensions is the same
    if (a->dimensions != b->dimensions) {
        return false;
    }

    // Check if the shape of each dimension is the same
    for (int i = 0; i < a->dimensions; i++) {
        if (a->shape[i] != b->shape[i]) {
            return false;
        }
    }

    // Check if the strides of each dimension is the same
    for (int i = 0; i < a->dimensions; i++) {
        if (a->strides[i] != b->strides[i]) {
            return false;
        }
    }

    // Check if the org_strides of each dimension is the same
    for (int i = 0; i < a->dimensions; i++) {
        if (a->org_strides[i] != b->org_strides[i]) {
            return false;
        }
    }

    // If all checks passed, the tensors are equal
    return true;
}

// Print info of a tensor
void __print_info(
    Tensor *tensor,
    int dim,
    int* index
) {
    if (tensor->dimensions == 1 && tensor->shape[0] == 1) {
        printf("[%.4f]", tensor->data[0]);
    } else if (dim < tensor->dimensions - 1) {
        printf("[");
        for (int i = 0; i < tensor->shape[dim]; i++) {
            index[dim] = i; 
            __print_info(tensor, dim + 1, index);
            if (i < tensor->shape[dim] - 1) {
                printf(",\n");
                for (int j = 0; j < tensor->dimensions - 2 - dim; j++) {
                    printf("\n");
                }
                
            }
        }
        printf("]");
    } else {
        printf("[");
        for (int i = 0; i < tensor->shape[dim]; i++) {
            index[dim] = i;
            int flat_idx = get_flat_index(tensor, index);

            if (i == tensor->shape[dim] - 1) {
                printf("%.4f", tensor->data[flat_idx]);
            } else {
                printf("%.4f, ", tensor->data[flat_idx]);
            }
        }
        printf("]");
    }
}

// Print info of a tensor
void print_info(
    Tensor *tensor
) {
    int *index = (int *)malloc(tensor->dimensions * sizeof(int));
    for (int i = 0; i < tensor->dimensions; i++) {
        index[i] = 0;
    }
    
    printf("[");
    for (int i = 0; i < tensor->shape[0]; i++) {
        index[0] = i;
        __print_info(tensor, 1, index);
        if (i < tensor->shape[0] - 1) {
            printf(",\n");
            for (int j = 0; j < tensor->dimensions - 2; j++) {
                printf("\n");
            }
        }
    }
    printf("]\n\n");


    printf("(");
    for (int i = 0; i < tensor->dimensions; i++) {
        if (i < tensor->dimensions - 1) {
            printf("%d,", tensor->shape[i]);
        } else {
            printf("%d", tensor->shape[i]);
        }
    }
    printf(")\n\n");
    
    free(index);
}

void print(
    int *arr,
    int count
) {
    for (int i = 0; i < count; i++) {
        printf("%d", arr[i]);
        if (i < count - 1) {
            printf(", ");
        }
    }
    printf("\n\n");
}

// Function to broadcast tensors so that they would align each other
void broadcast(
    Tensor *a,
    Tensor *b,
    int num_preserved_dims_a,
    int *preserved_dims_a,
    int num_preserved_dims_b,
    int *preserved_dims_b,
    Tensor **out_a,
    Tensor **out_b
) {
    // Determine the maximum number of dimensions
    int max_dims = (a->dimensions > b->dimensions) ? a->dimensions : b->dimensions;
    int offset_dims = (a->dimensions > b->dimensions) ? a->dimensions - b->dimensions : b->dimensions - a->dimensions;

    // Allocate memory for broadcasted shapes
    int *broadcast_shape_a = (int *)malloc(max_dims * sizeof(int));
    int *broadcast_shape_b = (int *)malloc(max_dims * sizeof(int));

    // Arrays to store preserved situation of dimensions (initialize all as false)
    bool *state_dims_a = (bool *)calloc(max_dims, sizeof(bool));
    bool *state_dims_b = (bool *)calloc(max_dims, sizeof(bool));

    // Identify preserved dimensions
    for (int i = 0; i < num_preserved_dims_a; i++) {
        int dim_to_preserve = preserved_dims_a[i];
        if (a->dimensions < max_dims) {
            state_dims_a[offset_dims + dim_to_preserve] = true;
        } else {
            state_dims_a[dim_to_preserve] = true;
        }
    }
    
    for (int i = 0; i < num_preserved_dims_b; i++) {
        int dim_to_preserve = preserved_dims_b[i];
        if (b->dimensions < max_dims) {
            state_dims_b[offset_dims + dim_to_preserve] = true;
        } else {
            state_dims_b[dim_to_preserve] = true;
        }
    }

    // Fill in the shapes starting from the leftmost dimension
    for (int i = 0; i < max_dims; i++) {
        int dim_a = (i >= max_dims - a->dimensions) ? a->shape[i - (max_dims - a->dimensions)] : 1;
        int dim_b = (i >= max_dims - b->dimensions) ? b->shape[i - (max_dims - b->dimensions)] : 1;

        // Determine the broadcasted dimension size, only if the dimension is not preserved
        if (state_dims_a[i]) {
            broadcast_shape_a[i] = dim_a;
        } else {
            // Apply regular broadcasting rules
            if (dim_a == dim_b) {
                broadcast_shape_a[i] = dim_a;
            } else if (dim_a > 1 && dim_b == 1) {
                broadcast_shape_a[i] = dim_a;
            } else if (dim_a == 1) {
                broadcast_shape_a[i] = dim_b;
            } else {
                fprintf(stderr, "Error: Tensors are not broadcastable.\n");
                exit(EXIT_FAILURE);
            }
        }

        if (state_dims_b[i]) {
            broadcast_shape_b[i] = dim_b;
        } else {
            // Apply regular broadcasting rules
            if (dim_a == dim_b) {
                broadcast_shape_b[i] = dim_b;
            } else if (dim_b > 1 && dim_a == 1) {
                broadcast_shape_b[i] = dim_b;
            } else if (dim_b == 1) {
                broadcast_shape_b[i] = dim_a;
                dim_b = dim_a;
            } else {
                fprintf(stderr, "Error: Tensors are not broadcastable.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Create the output tensors with the broadcasted shape
    create_tensor(broadcast_shape_a, max_dims, out_a);
    create_tensor(broadcast_shape_b, max_dims, out_b);

    // Broadcast tensor a and fill in tensor out_a
    int offset_a = max_dims - a->dimensions;
    int *src_idx_a = (int *)malloc(a->dimensions * sizeof(int));
    int *dest_idx_a = (int *)malloc((*out_a)->dimensions * sizeof(int));

    for (int i = 0; i < (*out_a)->total_size; i++) {
        get_multi_dimensional_index(*out_a, i, dest_idx_a);

        for (int j = offset_a; j < max_dims; j++) {
            int orig_idx = j - offset_a;
            src_idx_a[orig_idx] = (a->shape[orig_idx] > 1) ? dest_idx_a[j] : 0;
        }

        int flat_src_idx = get_flat_index(a, src_idx_a);
        double ref_value = a->data[flat_src_idx];
        (*out_a)->data[i] = ref_value;
    }

    free(src_idx_a);
    free(dest_idx_a);

    // Broadcast tensor b and fill in tensor out_b
    int offset_b = max_dims - b->dimensions;
    int *src_idx_b = (int *)malloc(b->dimensions * sizeof(int));
    int *dest_idx_b = (int *)malloc((*out_b)->dimensions * sizeof(int));

    for (int i = 0; i < (*out_b)->total_size; i++) {
        get_multi_dimensional_index(*out_b, i, dest_idx_b);

        for (int j = offset_b; j < max_dims; j++) {
            int orig_idx = j - offset_b;
            src_idx_b[orig_idx] = (b->shape[orig_idx] > 1) ? dest_idx_b[j] : 0;
        }
        
        int flat_src_idx = get_flat_index(b, src_idx_b);
        double ref_value = b->data[flat_src_idx];
        (*out_b)->data[i] = ref_value;
    }

    free(src_idx_b);
    free(dest_idx_b);

    // Free allocated memory
    free(broadcast_shape_a);
    free(broadcast_shape_b);
    free(state_dims_a);
    free(state_dims_b);
}

double ct_add(
    double a,
    double b,
    double *out_grad_a,
    double *out_grad_b
) {
    double out = a + b;

    if (out_grad_a) *out_grad_a = 1;
    if (out_grad_b) *out_grad_b = 1;
    
    return out;
}

double ct_sub(
    double a,
    double b,
    double *out_grad_a,
    double *out_grad_b
) { 
    double out = a - b;
    if (out_grad_a) *out_grad_a = 1;
    if (out_grad_b) *out_grad_b = -1;
    
    return out;
}

double ct_mul(
    double a,
    double b,
    double *out_grad_a,
    double *out_grad_b
) {
    double out = a * b;
    if (out_grad_a) *out_grad_a = b;
    if (out_grad_b) *out_grad_b = a;
    
    return out;
}

double ct_pow(
    double a,
    double b,
    double *out_grad_a,
    double *out_grad_b
) {
    double out = pow(a, b);
    if (out_grad_a) *out_grad_a = b * pow(a, b - 1);

    return out;
}

double ct_div(
    double a,
    double b,
    double *out_grad_a,
    double *out_grad_b
) {
    if (b == 0) {
        fprintf(stderr, "Error: Division by zero.\n");
        exit(EXIT_FAILURE);
    }

    double out = a / b;
    if (out_grad_a) *out_grad_a = 1 / b;
    if (out_grad_b) *out_grad_b = -a / (b * b);

    return out;
}

double ct_neg(
    double a,
    double *out_grad_a
) {
    double out = -a;
    if (out_grad_a) *out_grad_a = -1;

    return out;
}

double ct_tanh(
    double a,
    double *out_grad_a
) {
    double e_pos = exp(a);
    double e_neg = exp(-a);
    double out = (e_pos - e_neg) / (e_pos + e_neg);
    if (out_grad_a) *out_grad_a = 1 - (out * out);

    return out;
}

double ct_tan(
    double a,
    double *out_grad_a
) {
    double out = tan(a);
    double c = cos(a);
    if (out_grad_a) *out_grad_a = 1.0 / (c * c);

    return out;
}

double ct_abs(
    double a,
    double *out_grad_a
) {
    double out = a >= 0 ? a : -a;
    if (out_grad_a) *out_grad_a = a >= 0 ? 1 : -1;

    return out;
}

double ct_relu(
    double a,
    double *out_grad_a
) {
    double out = a > 0 ? a : 0;
    if (out_grad_a) *out_grad_a = a > 0 ? 1 : 0;

    return out;
}

double ct_pow2(
    double a,
    double *out_grad_a
) {
    double out = a * a;
    *out_grad_a = 2 * a;

    return out;
}

// Function to perform matrix multiplication on 2D arrays
void matrix_multiply(
    double *a,
    double *b,
    double *out,
    int n,
    int m,
    int p
) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            out[i * p + j] = 0;
            for (int k = 0; k < m; k++) {
                out[i * p + j] += a[i * m + k] * b[k * p + j];
            }
        }
    }
}

void sum(
    Tensor *tensor,
    Tensor **out_grad_tensor,
    Tensor **out_tensor
) {
    create_tensor_from_scalar(0.0, out_tensor);
    
    for (int i = 0; i < tensor->total_size; i++) {
        (*out_tensor)->data[0] += tensor->data[i];
    }

    deep_copy_tensor(tensor, out_grad_tensor);
    init_tensor(*out_grad_tensor, 1.0);
}

// Function to perform batch matrix multiplication
void matmul(
    Tensor *a,
    Tensor *b,
    Tensor **out_result
) {
    // Dimensions of the matrices to multiply
    int a_last_dim = a->shape[a->dimensions - 1];
    int a_second_last_dim = (a->dimensions > 1) ? a->shape[a->dimensions - 2] : 1;
    int b_last_dim = b->shape[b->dimensions - 1];
    int b_second_last_dim = (b->dimensions > 1) ? b->shape[b->dimensions - 2] : 1;

    // Ensure matrix multiplication dimensions match
    if (a_last_dim != b_second_last_dim) {
        fprintf(stderr, "Error: Matrix multiplication dimensions do not align.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate the number of batch dimensions for each tensor
    int a_batch_dims = a->dimensions - 2;
    int b_batch_dims = b->dimensions - 2;

    // Broadcasted batch dimensions for both tensors and preserve the last two dimensions (matrix dimensions)
    Tensor *broadcasted_a = NULL;
    Tensor *broadcasted_b = NULL;
    int preserved_dims_a[] = {a->dimensions - 1, a->dimensions - 2};
    int preserved_dims_b[] = {b->dimensions - 1, b->dimensions - 2};
    broadcast(a, b, 2, preserved_dims_a, 2, preserved_dims_b, &broadcasted_a, &broadcasted_b);

    // Create the output tensor
    int *out_shape = (int *)malloc((broadcasted_a->dimensions) * sizeof(int));
    memcpy(out_shape, broadcasted_a->shape, (broadcasted_a->dimensions - 2) * sizeof(int));
    out_shape[broadcasted_a->dimensions - 2] = a_second_last_dim;  // n from a
    out_shape[broadcasted_a->dimensions - 1] = b_last_dim;         // p from b
    create_tensor(out_shape, broadcasted_a->dimensions, out_result);

    // Iterate over the broadcasted batch dimensions
    for (int i = 0; i < calculate_total_size(broadcasted_a->shape, broadcasted_a->dimensions - 2); i++) {
        // Identify the correct slices for 'a' and 'b'
        int a_batch_idx = i * a_second_last_dim * a_last_dim;
        int b_batch_idx = i * b_second_last_dim * b_last_dim;

        double *a_slice = &broadcasted_a->data[a_batch_idx];
        double *b_slice = &broadcasted_b->data[b_batch_idx];
        double *out_slice = &(*out_result)->data[i * a_second_last_dim * b_last_dim];

        // Perform matrix multiplication for this slice
        matrix_multiply(a_slice, b_slice, out_slice, a_second_last_dim, a_last_dim, b_last_dim);
    }

    // Free allocated memory
    free(out_shape);
    free_tensor(broadcasted_a, true);
    free_tensor(broadcasted_b, true);
}

// Function to transpose a tensor by swapping two dimensions
void transpose_tensor(
    Tensor *tensor,
    int dim1,
    int dim2,
    Tensor **out_tensor
) {
    int *shape = tensor->shape;
    int dimensions = tensor ->dimensions;

    // Create a new tensor structure for the transpose
    create_tensor(shape, dimensions, out_tensor);
    
    // We do not need tensor_T->data because we are going to use tensor->data instead
    free((*out_tensor)->data);
    
    // Swap the specified dimensions in the shape array
    int temp_shape = (*out_tensor)->shape[dim1];
    (*out_tensor)->shape[dim1] = (*out_tensor)->shape[dim2];
    (*out_tensor)->shape[dim2] = temp_shape;
    
    // Swap the strides for the transposed dimensions
    int temp_stride = (*out_tensor)->strides[dim1];
    (*out_tensor)->strides[dim1] = (*out_tensor)->strides[dim2];
    (*out_tensor)->strides[dim2] = temp_stride;

    // Re-calculate the original strides for the transposed tensor
    calculate_strides((*out_tensor)->shape, (*out_tensor)->dimensions, (*out_tensor)->org_strides);
    
    // The data pointer is shared between input and result tensors
    (*out_tensor)->data = tensor->data;
}

// Define the operation
void operation(
    void *op,   // This is a function pointer, could point to various types of operations
    Tensor *a,
    Tensor *b,
    int dim1,
    int dim2,
    Tensor **out_grad_a,
    Tensor **out_grad_b,
    Tensor **out_result
) {
    // Check if the operation is a single-argument double operation (single tensor element-wise)
    if (op != NULL && b == NULL) {
        if ((void (*)(Tensor *, int, int, Tensor **))op == transpose_tensor) {
            transpose_tensor(a, dim1, dim2, out_result);
        } else if ((void (*)(Tensor *, Tensor **, Tensor **))op == sum) {
            sum(a, out_grad_a, out_result);
        } else {
            // Single-tensor operation
            double (*op_single)(double, double *) = (double (*)(double, double *))op;
            create_tensor(a->shape, a->dimensions, out_result);
            if (out_grad_a != NULL) {
                // Create gradient tensor
                create_tensor((*out_result)->shape, (*out_result)->dimensions, out_grad_a);
                for (int i = 0; i < a->total_size; i++) {
                    (*out_result)->data[i] = op_single(a->data[i], &(*out_grad_a)->data[i]);
                }
            } else {
                for (int i = 0; i < a->total_size; i++) {
                    (*out_result)->data[i] = op_single(a->data[i], NULL);
                }
            }
        }
    } else if (op != NULL && b != NULL) {
        // Check if the operation is matrix multiplication or element-wise tensor operation
        if ((void (*)(Tensor *, Tensor *, Tensor **))op == matmul) {
            // Batch matrix multiplication
            matmul(a, b, out_result);
        } else {
            // Two-tensor operation with broadcasting
            double (*op_double)(double, double, double *, double *) = (double (*)(double, double, double *, double *))op;
            Tensor *broadcasted_a = NULL;
            Tensor *broadcasted_b = NULL;
            broadcast(a, b, 0, NULL, 0, NULL, &broadcasted_a, &broadcasted_b);

            create_tensor(broadcasted_a->shape, broadcasted_a->dimensions, out_result);

            if (out_grad_a != NULL && out_grad_b != NULL) {
                // Both gradients required
                create_tensor((*out_result)->shape, (*out_result)->dimensions, out_grad_a);
                create_tensor((*out_result)->shape, (*out_result)->dimensions, out_grad_b);
                for (int i = 0; i < (*out_result)->total_size; i++) {
                    (*out_result)->data[i] = op_double(broadcasted_a->data[i], broadcasted_b->data[i], &(*out_grad_a)->data[i], &(*out_grad_b)->data[i]);
                }
            } else if (out_grad_a != NULL) {
                // Only out_grad_a required
                create_tensor((*out_result)->shape, (*out_result)->dimensions, out_grad_a);
                for (int i = 0; i < (*out_result)->total_size; i++) {
                    (*out_result)->data[i] = op_double(broadcasted_a->data[i], broadcasted_b->data[i], &(*out_grad_a)->data[i], NULL);
                }
            } else if (out_grad_b != NULL) {
                // Only out_grad_b required
                create_tensor((*out_result)->shape, (*out_result)->dimensions, out_grad_b);
                for (int i = 0; i < (*out_result)->total_size; i++) {
                    (*out_result)->data[i] = op_double(broadcasted_a->data[i], broadcasted_b->data[i], NULL, &(*out_grad_b)->data[i]);
                }
            } else {
                // No gradients required
                for (int i = 0; i < (*out_result)->total_size; i++) {
                    (*out_result)->data[i] = op_double(broadcasted_a->data[i], broadcasted_b->data[i], NULL, NULL);
                }
            }

            // Free broadcasted tensors
            free_tensor(broadcasted_a, true);
            free_tensor(broadcasted_b, true);
        }
    } else {
        // Handle the default error case
        fprintf(stderr, "Error: Unsupported operation or input types.\n");
        exit(EXIT_FAILURE);
    }
}

/////////////////////////////////////////////////////////////////////

enum NodeType {
    INPUT_OTHER,
    ONLY_INPUT,
    ONLY_OTHER
};

typedef struct Node {
    enum NodeType node_type;
    
    void *op;
    
    int dim1;
    int dim2;
    
    bool is_other_parameter;
    
    Tensor *input;
    Tensor *other;
    Tensor *output;

    Tensor *grad_input;
    Tensor *grad_other;
    Tensor *grad_output;

    // Pointers for linked list structure
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct {
    Node *first_node;
    Node *last_node;
} DCG;

void create_node(
    enum NodeType node_type,
    void *op,
    int dim1,
    int dim2,
    bool is_other_parameter,
    Node **out_node
) {
    // Dynamically allocate memory for the new node
    *out_node = (Node *)malloc(sizeof(Node));
    
    // Initialize node with the proper values
    (*out_node)->node_type = node_type;

    (*out_node)->op = op;

    (*out_node)->dim1 = dim1;
    (*out_node)->dim2 = dim2;

    (*out_node)->is_other_parameter = is_other_parameter;

    (*out_node)->input = NULL;
    (*out_node)->other = NULL;
    (*out_node)->output = NULL;

    (*out_node)->grad_input = NULL;
    (*out_node)->grad_other = NULL;
    (*out_node)->grad_output = NULL;

    (*out_node)->next = NULL;
    (*out_node)->prev = NULL;
}

void add_node(
    DCG *graph,
    Node *node,
    Tensor *other
) {
    node->other = other;

    node->next = NULL;
    node->prev = graph->last_node;

    // If this is the first node, initialize the first_node pointer
    if (graph->first_node == NULL) {
        graph->first_node = node;
    } else {
        // If not the first node, link the last node's next pointer to this node
        graph->last_node->next = node;
    }
    
    // Update the last_node to the new node
    graph->last_node = node;
}

void feedforward(
    Tensor *X,
    Node *from_node,
    Node *to_node,
    Tensor **out_tensor
) {
    Node *node = from_node;
    
    // Set the input to the current node
    node->input = X;

    // Traverse through the remaining nodes in the graph
    while (node != to_node->next) {
        // Perform the operation and store the result in the output tensor
        if (node->node_type == INPUT_OTHER) {
            operation(node->op, node->input, node->other, node->dim1, node->dim2, &node->grad_input, &node->grad_other, &node->output);
        } else if (node->node_type == ONLY_INPUT) {
            operation(node->op, node->input, NULL, node->dim1, node->dim2, &node->grad_input, NULL, &node->output);
        } else if (node->node_type == ONLY_OTHER) {
            operation(node->op, node->other, NULL, node->dim1, node->dim2, &node->grad_other, NULL, &node->output);
        }
        
        if (node->next) {
            if (node->node_type == ONLY_OTHER) {
                node->next->other = node->output;
                node->next->input = node->input;
            } else {
                node->next->input = node->output;
            }
        }

        // /////////
        // printf("Input:\n\n");
        // print_info(node->input);
        // printf("Other:\n\n");
        // if (node->other) print_info(node->other);

        // if (node->op == ct_add) printf("((((((((((+))))))))))\n\n");
        // if (node->op == ct_sub) printf("((((((((((-))))))))))\n\n");
        // if (node->op == ct_mul) printf("((((((((((x))))))))))\n\n");
        // if (node->op == ct_pow) printf("((((((((((^))))))))))\n\n");
        // if (node->op == ct_div) printf("((((((((((/))))))))))\n\n");
        // if (node->op == ct_neg) printf("((((((((((neg))))))))))\n\n");
        // if (node->op == ct_tanh) printf("((((((((((tanh))))))))))\n\n");
        // if (node->op == ct_tan) printf("((((((((((tan))))))))))\n\n");
        // if (node->op == ct_abs) printf("((((((((((abs))))))))))\n\n");
        // if (node->op == ct_relu) printf("((((((((((relu))))))))))\n\n");
        // if (node->op == sum) printf("((((((((((sum))))))))))\n\n");
        // if (node->op == matmul) printf("((((((((((@))))))))))\n\n");
        // if (node->op == transpose_tensor) printf("((((((((((trans))))))))))\n\n");

        // printf("Output:\n\n");
        // print_info(node->output);
        // printf("===>>>\n\n");
        // /////////
        
        node = node->next;
    }

    // Return the output of the last node
    *out_tensor = to_node->output;
}

void backward(DCG *graph, Tensor *lr) {
    Node *node = graph->last_node;

    // Allocate memory for the output tensor
    if (!node->grad_output) {
        create_tensor(node->output->shape, node->output->dimensions, &node->grad_output);
        init_tensor(node->grad_output, 1.0);
    }

    while (node) {
        if (node->op == matmul) {
            Tensor *input_T = NULL;
            Tensor *other_T = NULL;
            transpose_tensor(node->input, node->dim1, node->dim2, &input_T);
            transpose_tensor(node->other, node->dim1, node->dim2, &other_T);
            operation(matmul, node->grad_output, other_T, node->dim1, node->dim2, NULL, NULL, &node->grad_input);
            operation(matmul, input_T, node->grad_output, node->dim1, node->dim2, NULL, NULL, &node->grad_other);
        } else if (node->op == transpose_tensor) {
            if (node->node_type == ONLY_INPUT) {
                transpose_tensor(node->grad_output, node->dim1, node->dim2, &node->grad_input);
            } else if (node->node_type == ONLY_OTHER) {
                transpose_tensor(node->grad_output, node->dim1, node->dim2, &node->grad_other);
            }
        } else {
            Tensor *grad_input_tmp = NULL;
            operation(ct_mul, node->grad_input, node->grad_output, node->dim1, node->dim2, NULL, NULL, &grad_input_tmp);
            memcpy(node->grad_input->data, grad_input_tmp->data, grad_input_tmp->total_size * sizeof(double));
            if (node->grad_other) {
                Tensor *grad_other_tmp = NULL;
                operation(ct_mul, node->grad_other, node->grad_output, node->dim1, node->dim2, NULL, NULL, &grad_other_tmp);
                memcpy(node->grad_other->data, grad_other_tmp->data, grad_other_tmp->total_size * sizeof(double));
            }
        }

        if (node->is_other_parameter) {
            Tensor *delta = NULL;
            Tensor *other_tmp = NULL;
            operation(ct_mul, lr, node->grad_other, node->dim1, node->dim2, NULL, NULL, &delta);
            operation(ct_sub, node->other, delta, node->dim1, node->dim2, NULL, NULL, &other_tmp);
            memcpy(node->other->data, other_tmp->data, other_tmp->total_size * sizeof(double));
        }

        if (node->prev){
            if (node->prev->node_type == ONLY_OTHER) {
            node->prev->grad_output = node->grad_other;
            node->prev->grad_input = node->grad_input;
            } else {
                node->prev->grad_output = node->grad_input;
            }
        }

        node = node->prev;
    }
}

void free_graph(DCG *graph) {
    //TODO:
    Node *node = graph->first_node;
    while (node != NULL) {
        Node *next_node = node->next;
        
        // Free the dynamically allocated output tensor
        if (node->output != NULL) {
            free(node->output);
        }

        // Free the node itself
        free(node);
        
        node = next_node;
    }
    
    // Reset graph pointers
    graph->first_node = NULL;
    graph->last_node = NULL;
}

/////////////////////////////////////////////////////////////////////

typedef struct
{
    bool initiated;

    bool bias;
    int input_feature_size;
    int output_feature_size;

    Tensor *W;
    Tensor *b;

    Node *from_node;
    Node *to_node;
} LinearLayer;

LinearLayer *linearlayer(
    int input_feature_size,
    int output_feature_size,
    bool bias
) {
    LinearLayer *ll = (LinearLayer *)malloc(sizeof(LinearLayer));

    ll->initiated = false;
    ll->bias = bias;
    ll->input_feature_size = input_feature_size;
    ll->output_feature_size = output_feature_size;

    return ll;
}

void forward_linearlayer(
    DCG *graph,
    LinearLayer *io_ll,
    Tensor *X,
    Tensor **out_tensor
) {
    if (io_ll->initiated == false) {
        create_tensor((int[]) {io_ll->output_feature_size, io_ll->input_feature_size}, 2, &io_ll->W);
        init_tensor_rand(io_ll->W);
        Node *W_T_node = NULL;
        create_node(ONLY_OTHER, transpose_tensor, 0, 1, true, &W_T_node);
        io_ll->from_node = W_T_node;
        add_node(graph, W_T_node, io_ll->W);

        Node *X_W_T_node = NULL;
        create_node(INPUT_OTHER, matmul, 0, 1, false, &X_W_T_node);
        add_node(graph, X_W_T_node, NULL);

        if (io_ll->bias == true) {
            create_tensor((int[]) {X->shape[0], io_ll->output_feature_size}, 2, &io_ll->b);
            init_tensor_rand(io_ll->b);
            Node *b_node = NULL;
            create_node(INPUT_OTHER, ct_add, 0, 1, true, &b_node);
            add_node(graph, b_node, io_ll->b);
            io_ll->to_node = b_node;
        } else {
            io_ll->to_node = X_W_T_node;
        }
        
        io_ll->initiated = true;
    }
    
    feedforward(X, io_ll->from_node, io_ll->to_node, out_tensor);
}

void free_linearlayer(LinearLayer *ll) {
    free_tensor(ll->W, true);
    if (ll->bias) free_tensor(ll->b, true);
}

/////////////////////////////////////////////////////////////////////

typedef struct
{
    bool initiated;

    void *op;

    Node *from_node;
    Node *to_node;
} ActivationLayer;

ActivationLayer *activation_layer(
    void *op
) {
    ActivationLayer *al = (ActivationLayer *)malloc(sizeof(ActivationLayer));
    al->op = op;
    al->initiated = false;

    return al;
}

void forward_activationlayer(
    DCG *graph,
    ActivationLayer *al,
    Tensor *X,
    Tensor **out_tensor
) {
    if (!al->initiated) {
        Node *act_node = NULL;
        create_node(ONLY_INPUT, al->op, 0, 1, false, &act_node);
        add_node(graph, act_node, NULL);
        al->from_node = act_node;
        al->to_node = act_node;

        al->initiated = true;
    }

    feedforward(X, al->from_node, al->to_node, out_tensor);
}

void free_activationlayer(ActivationLayer *activationlayer) {
    free(activationlayer);
}

/////////////////////////////////////////////////////////////////////

typedef struct {
    bool initiated;

    Tensor *n;
    Tensor *two;
    Tensor *cur_mse;

    Node *from_node;
    Node *to_node;
} MSELoss;

MSELoss *mseloss(
    int N
) {
    MSELoss *mseloss = (MSELoss *)malloc(sizeof(MSELoss));

    mseloss->initiated = false;
    
    create_tensor_from_scalar(1.0 / N, &mseloss->n);
    create_tensor_from_scalar(2.0, &mseloss->two);
    create_tensor_from_scalar(0.0, &mseloss->cur_mse);

    return mseloss;
}

void forward_mseloss(
    DCG *graph,
    MSELoss *mseloss,
    Tensor *X,
    Tensor *target,
    Tensor **out_tensor
) {
    if (!mseloss->initiated) {
        Node *sub_node = NULL;
        create_node(INPUT_OTHER, ct_sub, 0, 1, false, &sub_node);
        add_node(graph, sub_node, target);
        mseloss->from_node = sub_node;

        Node *sub_pow_node = NULL;
        create_node(INPUT_OTHER, ct_pow, 0, 1, false, &sub_pow_node);
        add_node(graph, sub_pow_node, mseloss->two);

        Node *sub_pow_mul_node = NULL;
        create_node(INPUT_OTHER, ct_mul, 0, 1, false, &sub_pow_mul_node);
        add_node(graph, sub_pow_mul_node, mseloss->n);

        Node *sub_pow_mul_sum_node = NULL;
        create_node(ONLY_INPUT, sum, 0, 1, false, &sub_pow_mul_sum_node);
        add_node(graph, sub_pow_mul_sum_node, NULL);
        mseloss->to_node = sub_pow_mul_sum_node;

        mseloss->initiated = true;
    }

    feedforward(X, mseloss->from_node, mseloss->to_node, out_tensor);
}

void free_mseloss(MSELoss *mseloss) {
    free_tensor(mseloss->cur_mse, true);
    free_tensor(mseloss->two, true);
    free_tensor(mseloss->n, true);
    free(mseloss);
}

/////////////////////////////////////////////////////////////////////

#define NUM_IMAGES 20000  // Adjust this number if your dataset size differs
#define IMAGE_SIZE 784    // Each image has 28x28 pixels

double **mnist_images;  // Array of size [NUM_IMAGES][784] for training images
int *mnist_labels;      // Array of size [NUM_IMAGES] for training labels

// Helper function to allocate memory for MNIST images and labels
void allocate_mnist_memory(
    int num_images,
    int image_size
) {
    // Allocate memory for the mnist_images array (2D array)
    mnist_images = (double **)malloc(num_images * sizeof(double *));
    for (int i = 0; i < num_images; i++) {
        mnist_images[i] = (double *)malloc(image_size * sizeof(double));
    }

    // Allocate memory for the mnist_labels array (1D array)
    mnist_labels = (int *)malloc(num_images * sizeof(int));
}

// Function to load the MNIST dataset from a CSV file
void load_mnist_dataset(
    const char *mnist_csv_file
) {
    FILE *file = fopen(mnist_csv_file, "r");
    if (file == NULL) {
        printf("Error: Unable to open file %s\n", mnist_csv_file);
        exit(1);
    }

    // Allocate memory for the dataset
    allocate_mnist_memory(NUM_IMAGES, IMAGE_SIZE);

    // Read each line in the CSV file (assuming the first column is the label)
    char line[4096];  // Large enough to hold a line with 785 values (label + 784 pixels)
    int image_idx = 0;

    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");  // Tokenize the line by commas

        // First token is the label
        mnist_labels[image_idx] = atoi(token);

        // Read the next 784 tokens for pixel values
        for (int i = 0; i < IMAGE_SIZE; i++) {
            token = strtok(NULL, ",");
            if (token != NULL) {
                mnist_images[image_idx][i] = atof(token) / 255.0;  // Normalize pixel value
            }
        }

        image_idx++;

        // Stop if we have loaded enough images (for small datasets)
        if (image_idx >= NUM_IMAGES) {
            break;
        }
    }

    fclose(file);
}

// Helper function to one-hot encode the labels
void one_hot_encode(
    int label,
    double *encoded_label,
    int num_classes
) {
    for (int i = 0; i < num_classes; i++) {
        encoded_label[i] = (i == label) ? 1.0 : 0.0;
    }
}

// Function to load a batch of MNIST data into tensors X and Y
void load_mnist_batch(
    Tensor *X,
    Tensor *Y,
    int batch_idx,
    int batch_size
) {
    int start_idx = batch_idx * batch_size;

    // Loop through the batch
    for (int i = 0; i < batch_size; i++) {
        int data_idx = start_idx + i;

        // Load the image and flatten it into a 784-length vector
        for (int j = 0; j < 784; j++) {
            set_element(X, mnist_images[data_idx][j], i, j);  // Normalize the pixel value
        }

        // Load the label and one-hot encode it into a 10-length vector
        double one_hot_label[10] = {0};
        one_hot_encode(mnist_labels[data_idx], one_hot_label, 10);

        for (int j = 0; j < 10; j++) {
            set_element(Y, one_hot_label[j], i, j);
        }
    }
}

void mnist_test(
    DCG *graph,
    const char *mnist_csv_file
) {
    // Load the MNIST dataset from the CSV file
    load_mnist_dataset(mnist_csv_file);

    // Define hyperparameters
    int batch_size = 64;
    int num_batches = 300;  // Number of batches in the epoch (adjust accordingly)
    int epoch = 10;
    double lr = 0.001;       // Learning rate

    // Define DNN architecture: 784 -> 128 -> 64 -> 10
    int input_size = 784;  // Flattened MNIST image (28 * 28)
    int output_size = 10;  // MNIST has 10 classes (digits 0-9)

    // Placeholder for batch input and labels
    int X_shape[] = {batch_size, input_size};
    int Y_shape[] = {batch_size, output_size};
    Tensor *X = NULL, *Y = NULL;
    create_tensor(X_shape, 2, &X);  // Create (batch_size, 784) tensor
    create_tensor(Y_shape, 2, &Y);  // Create (batch_size, 10) tensor

    // Initialize DNN layers
    LinearLayer *ll1 = linearlayer(input_size, 128, false); // Input -> Hidden Layer 1
    ActivationLayer *al1 = activation_layer(ct_relu);       // Activation after Layer 1
    LinearLayer *ll2 = linearlayer(128, 64, false);         // Hidden Layer 1 -> Hidden Layer 2
    ActivationLayer *al2 = activation_layer(ct_relu);       // Activation after Layer 2
    LinearLayer *ll3 = linearlayer(64, output_size, false); // Hidden Layer 2 -> Output Layer

    MSELoss *mse = mseloss(output_size * batch_size);       // Loss layer (Mean Squared Error)

    Tensor *tensor_lr = NULL;
    create_tensor_from_scalar(lr, &tensor_lr);              // Learning rate as a scalar tensor

    Tensor *y1 = NULL, *y2 = NULL, *y3 = NULL, *y4 = NULL, *y_pred = NULL;
    Tensor *out_mse = NULL;

    // Assume load_mnist_batch loads the MNIST batch of images and labels
    // You will need to implement this function or load the data accordingly.
    
    for (int e = 1; e <= epoch; e++) {
        printf("Epoch: %d/%d\n", e, epoch);
        
        for (int b = 1; b <= num_batches; b++) {
            // Load a batch of data (X, Y)
            load_mnist_batch(X, Y, b, batch_size);  // You will need to implement this function

            // Forward pass through the DNN
            forward_linearlayer(graph, ll1, X, &y1);
            forward_activationlayer(graph, al1, y1, &y2);
            forward_linearlayer(graph, ll2, y2, &y3);
            forward_activationlayer(graph, al2, y3, &y4);
            forward_linearlayer(graph, ll3, y4, &y_pred);
            
            forward_mseloss(graph, mse, y_pred, Y, &out_mse);

            // Backward pass to update weights
            backward(graph, tensor_lr);

            // Every 100 batches, print the loss (MSE)
            if (b % 100 == 0) {
                printf("Batch %d/%d - MSE Loss:\n", b, num_batches);
                print_info(out_mse);  // Print the loss tensor
                printf("-------------------\n");
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////

#define SIMPLE_TEST_DATASET_SIZE 64000
#define SIMPLE_TEST_FEATURE_SIZE 1

void shuffle_data(
    Tensor *X, 
    Tensor *Y, 
    int dataset_size, 
    int feature_size
) {
    // Fisher-Yates shuffle algorithm
    for (int i = dataset_size - 1; i > 0; i--) {
        // Generate a random index j such that 0 <= j <= i
        int j = rand() % (i + 1);

        // Swap row i with row j in X
        for (int k = 0; k < feature_size; k++) {
            double temp = get_element(X, i, k);
            set_element(X, get_element(X, j, k), i, k);
            set_element(X, temp, j, k);
        }

        // Swap the corresponding row i with row j in Y
        for (int k = 0; k < feature_size; k++) {
            double temp = get_element(Y, i, k);
            set_element(Y, get_element(Y, j, k), i, k);
            set_element(Y, temp, j, k);
        }
    }
}

void load_simple_test_dataset(
    int dataset_size,
    int feature_size,
    Tensor **out_X,
    Tensor **out_Y
) {
    int X_shape[] = {dataset_size, feature_size};
    create_tensor(X_shape, 2, out_X);

    for (int i = 0; i < dataset_size; i++) {
        for (int j = 0; j < feature_size; j++) {
            double x = -32 + i * 0.001;
            x = x / 32.0;
            set_element(*out_X, x, i, j);
        }
    }
    
    int Y_shape[] = {dataset_size, feature_size};
    Tensor *Y = NULL;
    create_tensor(Y_shape, 2, out_Y);
    
    for (int i = 0; i < dataset_size; i++) {
        for (int j = 0; j < feature_size; j++) {
            double y = -32 + i * 0.001;
            y = y / 32.0;
            y = y * y;
            set_element(*out_Y, y, i, j);
        }
    }
    
    // Shuffle the data of out_X and keep out_Y in sync
    shuffle_data(*out_X, *out_Y, dataset_size, feature_size);
}

// Function to load a batch of simple_test data into tensors X and Y
void load_simple_test_batch(
    Tensor *X_dataset,
    Tensor *Y_dataset,
    int batch_idx,
    int batch_size,
    int feature_size,
    Tensor *io_X,
    Tensor *io_Y
) {
    int start_idx = batch_idx * batch_size;

    // Loop through the batch
    for (int i = 0; i < batch_size; i++) {
        int data_idx = start_idx + i;
        for (int j = 0; j < feature_size; j++) {
            int multi_dim_idx[2] = {i, j};
            int idx = get_flat_index(X_dataset, multi_dim_idx);
            set_element(io_X, X_dataset->data[idx], i, j);
            set_element(io_Y, Y_dataset->data[idx], i, j);
        }
    }
}

void simple_test(DCG *graph) {
    int dataset_size = SIMPLE_TEST_DATASET_SIZE;
    int feature_size = SIMPLE_TEST_FEATURE_SIZE;

    int batch_size = 64;
    int num_batches = 1000;
    int epoch = 1000;
    double lr = 0.001;

    Tensor *X_dataset = NULL, *Y_dataset = NULL;
    load_simple_test_dataset(dataset_size, feature_size, &X_dataset, &Y_dataset);

    // Placeholder for batch input and labels
    int X_shape[] = {batch_size, feature_size};
    int Y_shape[] = {batch_size, feature_size};
    Tensor *X = NULL, *Y = NULL;
    create_tensor(X_shape, 2, &X);
    create_tensor(Y_shape, 2, &Y);

    Tensor *tensor_lr = NULL;
    create_tensor_from_scalar(lr, &tensor_lr);

    LinearLayer *ll1 = linearlayer(X->shape[1], 1, false);
    LinearLayer *ll2 = linearlayer(1, Y->shape[1], false);
    ActivationLayer *al = activation_layer(ct_pow2);
    MSELoss *mse = mseloss(feature_size * batch_size);

    Tensor *y1 = NULL, *y2 = NULL, *y_pred = NULL;
    Tensor *out_mse = NULL;

    for (int i = 1; i < epoch + 1; i++) {
        printf("Epoch: %d/%d\n\n", i, epoch);

        for (int j = 0; j < num_batches + 1; j++) {
            load_simple_test_batch(X_dataset, Y_dataset, j, batch_size, feature_size, X, Y);

            forward_linearlayer(graph, ll1, X, &y1);
            forward_linearlayer(graph, ll2, y1, &y2);
            forward_activationlayer(graph, al, y2, &y_pred);
            forward_mseloss(graph, mse, y_pred, Y, &out_mse);

            backward(graph, tensor_lr);
        }
        
        printf("W1:\n");
        print_info(ll1->W);
        printf("W2:\n");
        print_info(ll2->W);
        printf("MSE Loss:\n");
        print_info(out_mse);  // Print the loss tensor
        printf("-------------------\n");
    }
}

/////////////////////////////////////////////////////////////////////

// Function to setup the whole application
DCG *setup_application(
    int default_seed
) {
    // Seed value for random number generation
    unsigned int seed = (default_seed < 0) ? time(NULL) * time(NULL) : default_seed;
    printf("Application Seed: %u\n\n", seed);

    // Seed the random number generator, usually done once per program run
    srand(seed);

    // Initialize DCG
    DCG *graph = (DCG *)malloc(sizeof(DCG));
    graph->first_node = NULL;
    graph->last_node = NULL;

    return graph;
}

int main(
    int argc,
    char *argv[]
) {
    DCG *graph = setup_application(42);

    // mnist_test(graph, "<CSV_PATH>");
    simple_test(graph);

    return 0;
}
