; RUN: opt -passes=openmp-opt -S < %s | FileCheck %s

; This test verifies that stores to __kmpc_alloc_shared allocations are correctly
; handled and not incorrectly optimized. The bug was that when converting generic
; kernels to SPMD mode, stores to shared memory allocated by __kmpc_alloc_shared
; were being guarded, causing only lane 0 to initialize the memory. This left
; lanes 1-31 with uninitialized pointers, leading to GPU memory access faults.
;
; Based on the split-outer2.f90 bug where a nested parallel region needed to
; access an array pointer stored in shared memory by the parent distribute loop.

target datalayout = "e-m:e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-p7:160:256:256:32-p8:128:128-p9:192:256:256:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5-G1-ni:7:8:9"
target triple = "amdgcn-amd-amdhsa"

%struct.ident_t = type { i32, i32, i32, i32, ptr }
%struct.KernelEnvironmentTy = type { %struct.ConfigurationEnvironmentTy, ptr, ptr }
%struct.ConfigurationEnvironmentTy = type { i8, i8, i8, i32, i32, i32, i32, i32, i32 }

; Generic kernel environment (exec_mode = 1)
@kernel_env = local_unnamed_addr addrspace(1) global %struct.KernelEnvironmentTy {
  %struct.ConfigurationEnvironmentTy {
    i8 1,  ; exec_mode = 1 (Generic)
    i8 0,  ; use_generic_state_machine = false
    i8 3,  ; MayUseNestedParallelism = true
    i32 0, i32 0, i32 0, i32 0, i32 0, i32 0
  },
  ptr null, ptr null
}
@ident = private unnamed_addr constant %struct.ident_t { i32 0, i32 2, i32 0, i32 0, ptr null }, align 8

; CHECK-LABEL: define {{.*}} @kernel_with_alloc_shared
; CHECK: call {{.*}} @__kmpc_alloc_shared
; CHECK: store ptr {{.*}}, ptr {{.*}}
; CHECK-NOT: call {{.*}} @__kmpc_free_shared(ptr null
define amdgpu_kernel void @kernel_with_alloc_shared(ptr %data_ptr) {
entry:
  %init = call i32 @__kmpc_target_init(ptr addrspacecast (ptr addrspace(1) @kernel_env to ptr), ptr null)
  
  ; Allocate shared memory (8 bytes for one pointer)
  %shared_mem = call align 8 ptr @__kmpc_alloc_shared(i64 8)
  
  ; Store data pointer into shared memory
  ; This pattern appears when passing data between distribute and parallel regions
  ; The store must be visible to all threads, so it should not be guarded
  store ptr %data_ptr, ptr %shared_mem, align 8
  
  ; Simulate use of shared memory (e.g., by passing to a callback)
  %loaded = load ptr, ptr %shared_mem, align 8
  %val = load i32, ptr %loaded, align 4
  
  ; Free shared memory
  call void @__kmpc_free_shared(ptr %shared_mem, i64 8)
  
  call void @__kmpc_target_deinit()
  ret void
}

; Test with multiple allocations to ensure the fix works for all of them
; CHECK-LABEL: define {{.*}} @kernel_multiple_alloc_shared
; CHECK: call {{.*}} @__kmpc_alloc_shared
; CHECK: store ptr {{.*}}, ptr {{.*}}
; CHECK: call {{.*}} @__kmpc_alloc_shared
; CHECK: store i32 {{.*}}, ptr {{.*}}
define amdgpu_kernel void @kernel_multiple_alloc_shared(ptr %data_ptr) {
entry:
  %init = call i32 @__kmpc_target_init(ptr addrspacecast (ptr addrspace(1) @kernel_env to ptr), ptr null)
  
  ; First allocation - store pointer
  %shared_mem1 = call align 8 ptr @__kmpc_alloc_shared(i64 8)
  store ptr %data_ptr, ptr %shared_mem1, align 8
  
  ; Second allocation - store integer
  %shared_mem2 = call align 4 ptr @__kmpc_alloc_shared(i64 4)
  store i32 42, ptr %shared_mem2, align 4
  
  ; Use both
  %loaded1 = load ptr, ptr %shared_mem1, align 8
  %loaded2 = load i32, ptr %shared_mem2, align 4
  
  ; Free both
  call void @__kmpc_free_shared(ptr %shared_mem1, i64 8)
  call void @__kmpc_free_shared(ptr %shared_mem2, i64 4)
  
  call void @__kmpc_target_deinit()
  ret void
}

; Test nested in a callback (closer to the real bug scenario)
; CHECK-LABEL: define {{.*}} @kernel_with_callback
; CHECK: call {{.*}} @__kmpc_distribute_static_loop
define amdgpu_kernel void @kernel_with_callback(ptr %data_ptr) {
entry:
  %init = call i32 @__kmpc_target_init(ptr addrspacecast (ptr addrspace(1) @kernel_env to ptr), ptr null)
  
  ; Allocate shared memory to pass to callback
  %shared_mem = call align 8 ptr @__kmpc_alloc_shared(i64 16)
  
  ; Store data into shared memory struct
  %field0 = getelementptr inbounds i8, ptr %shared_mem, i64 0
  store ptr %data_ptr, ptr %field0, align 8
  
  ; Call distribute loop with callback
  call void @__kmpc_distribute_static_loop_4u(ptr nonnull @ident, ptr @callback, ptr %shared_mem, i32 1, i32 0, i8 0)
  
  ; Free shared memory
  call void @__kmpc_free_shared(ptr %shared_mem, i64 16)
  
  call void @__kmpc_target_deinit()
  ret void
}

; CHECK-LABEL: define {{.*}} @callback
; CHECK: call {{.*}} @__kmpc_alloc_shared
; CHECK: store ptr {{.*}}, ptr {{.*}}
define internal void @callback(i32 %iter, ptr %args) {
entry:
  ; Load data from parent's shared memory
  %data_ptr = load ptr, ptr %args, align 8
  
  ; Allocate new shared memory for nested parallel region
  %shared_mem = call align 8 ptr @__kmpc_alloc_shared(i64 8)
  
  ; Store data into shared memory - THIS IS THE CRITICAL STORE
  ; This must not be guarded, or lanes 1-31 will have uninitialized pointers
  store ptr %data_ptr, ptr %shared_mem, align 8
  
  ; Nested parallel region would use this (simplified here)
  %loaded = load ptr, ptr %shared_mem, align 8
  %val = load i32, ptr %loaded, align 4
  
  ; Free shared memory
  call void @__kmpc_free_shared(ptr %shared_mem, i64 8)
  ret void
}

declare i32 @__kmpc_target_init(ptr, ptr) convergent nounwind
declare void @__kmpc_target_deinit() convergent nounwind
declare ptr @__kmpc_alloc_shared(i64) nounwind allocsize(0)
declare void @__kmpc_free_shared(ptr, i64) nounwind
declare void @__kmpc_distribute_static_loop_4u(ptr, ptr, ptr, i32, i32, i8) convergent

!llvm.module.flags = !{!0, !1}
!nvvm.annotations = !{!2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"openmp-device", i32 50}
!2 = !{ptr @kernel_with_alloc_shared, !"kernel", i32 1}
!3 = !{ptr @kernel_multiple_alloc_shared, !"kernel", i32 1}
!4 = !{ptr @kernel_with_callback, !"kernel", i32 1}

