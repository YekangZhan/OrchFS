#include "../config/config.h"
#include "device.h"

#ifdef __cplusplus       
extern "C"{
#endif

// #define MAX_WRITE_SIZE0               1024*1024*16
// void* alignbuf = NULL;

void ssd_init()
{
	// alignbuf = valloc(MAX_WRITE_SIZE0);

	fs_dev_info.ssd_read_fd = open("/dev/nvme7n1",  O_RDWR | O_SYNC); //
	// fs_dev_info.ssd_read_fd = open("/dev/nvme0n1",  O_RDWR | __O_DIRECT); //
    if (fs_dev_info.ssd_read_fd < 0) 
	{
        printf("open ssd error!\n");
        exit(1);
    }
	#ifdef NEWBASELINE
	fs_dev_info.ssd_write_fd = open("/dev/nvme7n1",  O_RDWR | O_SYNC); //| __O_DIRECT | O_SYNC
	#else
	fs_dev_info.ssd_write_fd = open("/dev/nvme7n1",  O_RDWR | __O_DIRECT); //
	#endif
	
    if (fs_dev_info.ssd_write_fd < 0) 
	{
        printf("open ssd error!\n");
        exit(1);
	}
}

void ssd_close() 
{
    if(fs_dev_info.ssd_read_fd > 0)
		close(fs_dev_info.ssd_read_fd);
	if(fs_dev_info.ssd_write_fd > 0)
		close(fs_dev_info.ssd_write_fd);
}

void* nvm_init(ioctl_init_t* frame)
{
    fs_dev_info.nvm_fd = open("/dev/dax0.0", O_RDWR);
	if(fs_dev_info.nvm_fd == -1)
    {
		printf("nvm init error!\n");
		exit(-1);
	}
	void * ret = mmap(NULL, PMEM_LEN ,PROT_WRITE | PROT_READ, MAP_SHARED ,fs_dev_info.nvm_fd ,0);
	if(ret == NULL)
    {
		printf("nvm init error! -- mmap error!\n");
		return NULL;
	}
    ioctl(fs_dev_info.nvm_fd, IOCTL_INIT, (uint64_t)frame);
    return ret;
}

void nvm_close()
{
    close(fs_dev_info.nvm_fd);
	munmap(fs_dev_info.nvm_base_addr,PMEM_LEN);
}

void ssd_read(void* dst, int64_t len, int64_t offset)
{
	int64_t readnum = pread(fs_dev_info.ssd_read_fd, dst, len, offset);
	if(readnum != len)
	{
		printf("read data error! %" PRId64" %" PRId64" %" PRId64"\n", readnum, len, offset);
		exit(1);
	}
}

// void ssd_read_pre(void* dst, int64_t len, int64_t offset)
// {
// 	int64_t end_byte = offset+len-1;
// 	int64_t start_offset_4k = (offset>>BW_4KiB), end_offset_4k = (end_byte>>BW_128KiB);
// 	int64_t read_offset = (start_offset_4k<<BW_4KiB);
	
// 	int64_t store_buf_size = (end_offset_4k+1)*SIZE_128KiB - start_offset_4k*SIZE_4KiB;

// 	void* store_buf = valloc(store_buf_size);
// 	int64_t readnum = pread(fs_dev_info.ssd_read_fd, store_buf, store_buf_size, read_offset);
// 	if(readnum != store_buf_size)
// 	{
// 		printf("read data error! %" PRId64" %" PRId64" %" PRId64"\n", readnum, len, offset);
// 		exit(1);
// 	}
// 	memcpy(dst, store_buf+offset-read_offset, len);
// 	free(store_buf);
// }


void shm_ssd_write(void* src, int64_t src_off, int64_t len, int64_t offset)
{
	assert(len % SIZE_4KiB == 0);
	int64_t writenum = pwrite(fs_dev_info.ssd_write_fd, src + src_off, len, offset);
	if(writenum != len)
	{
		printf("write data error! %" PRId64" %" PRId64" %" PRId64"\n", writenum, len, offset);
		exit(1);
	}
}


void ssd_write(void* src, int64_t len, int64_t offset)
{
	// printf("ssdw: %lld %lld\n",offset,len);
	uint64_t end_byte = offset+len-1;
	uint64_t start_offset_4k = (offset>>BW_4KiB);
	uint64_t end_offset_4k = (end_byte>>BW_4KiB);
	uint64_t block_sum = end_offset_4k-start_offset_4k+1;
	void* alignbuf = valloc(block_sum*SIZE_4KiB);
	uint64_t start_blk_pos = (offset&((1LL<<BW_4KiB)-1));
	uint64_t end_blk_pos = (end_byte&((1LL<<BW_4KiB)-1));
	if(start_blk_pos!=0 || (block_sum==1&&len < SIZE_4KiB))
	{
		int64_t readnum1 = pread(fs_dev_info.ssd_read_fd, alignbuf, SIZE_4KiB, offset);
		// printf("%" PRId64 " %" PRId64 "\n", readnum1, offset);
		assert(readnum1 == SIZE_4KiB);
	}
	if(block_sum > 1 && end_blk_pos+1 != SIZE_4KiB)
	{
		int64_t readnum2 = pread(fs_dev_info.ssd_read_fd, alignbuf+(block_sum-1)*SIZE_4KiB, SIZE_4KiB, offset+(block_sum-1)*SIZE_4KiB);
		// printf("%" PRId64 " %" PRId64 "\n", readnum2, offset);
		assert(readnum2 == SIZE_4KiB);
	}
	memcpy(alignbuf+start_blk_pos, src, len);
	if(block_sum > 1024*256)
	{
		int64_t max_wblks = 1024*256, alignbuf_off = 0;
		int64_t woffset = (start_offset_4k << BW_4KiB);
		while(block_sum > 0)
		{
			int64_t real_wblks = 0;
			if(block_sum > max_wblks)
				real_wblks = max_wblks;
			else
				real_wblks = block_sum;
			void* new_wpt = (void*)((int64_t)alignbuf + alignbuf_off);
			int64_t writenum = pwrite(fs_dev_info.ssd_write_fd, new_wpt, real_wblks*SIZE_4KiB, woffset + alignbuf_off);
			assert(writenum == real_wblks*SIZE_4KiB);
			block_sum -= real_wblks; alignbuf_off += real_wblks*SIZE_4KiB;
		}
	}
	else
	{
		int64_t woffset = (start_offset_4k << BW_4KiB);
		int64_t writenum = pwrite(fs_dev_info.ssd_write_fd, alignbuf, block_sum*SIZE_4KiB, woffset);
		// assert(writenum == block_sum*SIZE_4KiB);
	}
	free(alignbuf);
}

void avx_cpy(void *dest, const void *src, uint64_t size)
{
	/*
			* Copy the range in the forward direction.
			*
			* This is the most common, most optimized case, used unless
			* the overlap specifically prevents it.
			*/
	/* copy up to FLUSH_ALIGN boundary */

	size_t cnt = (uint64_t)dest & ALIGN_MASK;
	//printf("111111\n");
	if (unlikely(cnt > 0))
	{
		cnt = FLUSH_ALIGN - cnt;
		if(cnt > size){
			cnt = size;
			size = 0;
		}
		else{
			size -= cnt;
		}
		/* never try to copy more the len bytes */
		// register uint32_t d;
		register uint8_t d8;
		// while(cnt > 3){
		// 	d = *(uint32_t*)(src);
		// 	_mm_stream_si32(dest, d);
		// 	src += 4;
		// 	dest += 4;
		// 	cnt -= 4;
		// }
		// if(unlikely(cnt > 0)){
		while(cnt){
			d8 = *(uint8_t*)(src);
			*(uint8_t*)dest = d8;
			cnt --;
			src ++;
			dest ++;
		}
		cache_wb_one(dest);
		// }
		if(size == 0){
			return;
		}
	}
	//printf("222222\n");
	assert((uint64_t)dest % 64 == 0);
	register __m512i xmm0;
	while(size >= 64){
		xmm0 = _mm512_loadu_si512(src);
		//printf("xxxxx\n");
		_mm512_stream_si512(dest, xmm0);
		dest += 64;
		src += 64;
		size -= 64;
	}
	/* copy the tail (<512 bit)  */
	size &= ALIGN_MASK;
	if (unlikely(size != 0))
	{
		while(size > 0){
			*(uint8_t*)dest = *(uint8_t*)src;
			size --;
			dest ++;
			src ++;
		}
		cache_wb_one(dest - 1);
	}
}

void nvm_read(void* dst, int64_t len, int64_t offset)
{
	void* target_nvm_addr = (void*)(offset + (int64_t)fs_dev_info.nvm_base_addr);
	memcpy(dst, target_nvm_addr, len);
}

void nvm_write(void* src, int64_t len, int64_t offset)
{
	void* target_nvm_addr = (void*)(offset + (int64_t)fs_dev_info.nvm_base_addr);
	avx_cpy(target_nvm_addr, src, len);
}

void device_init()
{
	ssd_init();
	ioctl_init_t frame = {.size = (uint64_t)120<<30 } ;
    fs_dev_info.nvm_base_addr =  nvm_init(&frame);
	fs_dev_info.mpk[MPK_DEFAULT] = frame.mpk_default;
	fs_dev_info.mpk[MPK_FILE] = frame.mpk_file;
	fs_dev_info.mpk[MPK_META] = frame.mpk_meta;
	if(fs_dev_info.nvm_base_addr == NULL)
    {
		printf("Failed to init nvm\n");
		exit(1);
	}
}

void device_close()
{
	ssd_close();
	nvm_close();
}

void read_data_from_devs(void* dst, int64_t len, int64_t offset)
{
	if(offset + len <= PMEM_LEN)
	{
		nvm_read(dst, len, offset);
	}
	else if(offset < PMEM_LEN && offset + len > PMEM_LEN)
	{
		int64_t nvm_read_len = PMEM_LEN - offset;
		int64_t ssd_read_len = offset + len - PMEM_LEN;
		nvm_read(dst, nvm_read_len, offset);
		uint64_t ssd_read_buf_addr = (uint64_t)dst + nvm_read_len;
		void* ssd_read_buf_pt = (void*)ssd_read_buf_addr;
		ssd_read(ssd_read_buf_pt, ssd_read_len, 0);
	}
	else if(offset >= PMEM_LEN)
	{
		ssd_read(dst, len, offset - PMEM_LEN);
	}
}

void write_data_to_devs(void* src, int64_t len, int64_t offset)
{
	if(offset + len <= PMEM_LEN)
	{
		nvm_write(src, len, offset);
	}
	else if(offset < PMEM_LEN && offset + len > PMEM_LEN)
	{
		int64_t nvm_write_len = PMEM_LEN - offset;
		int64_t ssd_write_len = offset + len - PMEM_LEN;
		nvm_write(src, nvm_write_len, offset);
		uint64_t ssd_write_buf_addr = (uint64_t)src + nvm_write_len;
		void* ssd_write_buf_pt = (void*)ssd_write_buf_addr;
		ssd_write(ssd_write_buf_pt, ssd_write_len, 0);
	}
	else if(offset >= PMEM_LEN)
	{
		ssd_write(src, len, offset - PMEM_LEN);
	}
}

void shm_write_data_to_devs(void* src, int64_t src_off, int64_t len, int64_t offset)
{
	if(offset + len <= PMEM_LEN)
	{
		nvm_write(src + src_off, len, offset);
	}
	else if(offset < PMEM_LEN && offset + len > PMEM_LEN)
	{
		int64_t nvm_write_len = PMEM_LEN - offset;
		int64_t ssd_write_len = offset + len - PMEM_LEN;
		nvm_write(src + src_off, nvm_write_len, offset);
		uint64_t ssd_write_buf_addr = (uint64_t)src + nvm_write_len;
		void* ssd_write_buf_pt = (void*)ssd_write_buf_addr;
		shm_ssd_write(ssd_write_buf_pt, src_off, ssd_write_len, 0);
	}
	else if(offset >= PMEM_LEN)
	{
		shm_ssd_write(src, src_off, len, offset - PMEM_LEN);
	}
}

void write_data_to_ssds_newbaseline(void* src, int64_t len, int64_t offset)
{
	int64_t writenum = pwrite(fs_dev_info.ssd_write_fd, src, len, offset);
	return;
}

void read_data_from_ssds_newbaseline(void* dst, int64_t len, int64_t offset)
{
	int64_t readnum = pread(fs_dev_info.ssd_read_fd, dst, len, offset);
	return;
}

void write_data_to_nvms_newbaseline(void* src, int64_t len, int64_t offset)
{
	void* target_nvm_addr = (void*)(offset + (int64_t)fs_dev_info.nvm_base_addr + (50ll << 30) );
	avx_cpy(target_nvm_addr, src, len);
	return;
}

void read_data_from_nvms_newbaseline(void* dst, int64_t len, int64_t offset)
{
	void* target_nvm_addr = (void*)(offset + (int64_t)fs_dev_info.nvm_base_addr + (50ll << 30) );
	memcpy(dst, target_nvm_addr, len);
	return;
}


#ifdef __cplusplus
}
#endif