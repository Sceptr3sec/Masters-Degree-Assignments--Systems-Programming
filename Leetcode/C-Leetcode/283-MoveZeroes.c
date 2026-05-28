void moveZeroes(int* nums, int numsSize) {
    int n = 0;

    for(int i =0; i<numsSize; i++){
        if(nums[i] != 0){
            nums[n] = nums[i];
            n++;
        }
    }
    for(int i=n; i<numsSize; i++){
        nums[i] = 0;
    }
}