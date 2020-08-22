/***********************************
v1.3
Dmitry Bolshakov, April 2020
***********************************/
#pragma once
#ifndef PoolAllocator_H
#define PoolAllocator_H
#include <memory>
#include <cassert>
#define ALLOCATOR_VERIFY(cond, what) assert(cond && what)

template <class Ty>
class PoolAllocator {
public:
	using byte = unsigned char;																		
	using value_type = Ty;
private:
	struct MemoryPage {													//��������� ��������� �������� ������
		inline MemoryPage(size_t bytes_count, MemoryPage* link = nullptr) :  offset{ 0 }, size{ bytes_count }, prev{ link } {}
		size_t offset;													//offset � size - � ������, ������ ��������� �� �����������!
		size_t size;
		MemoryPage* prev;
	};
	struct FreeBlock {													//��������� ��������� �������������� �����
		inline FreeBlock(FreeBlock* link) : prev{ link } {}
		FreeBlock* prev;
	};
private:																	
	static const double reserve_multiplier;								//����������� ��������������								
	static const size_t min_allocated_blocks;							//����������� ����� ������ �� ��������
private:																//������� ���� ������, ���� ������, ����������� � ������ � �� ��������� ��������
	const size_t block_size{ sizeof(Ty) };
	const size_t header_size{ sizeof(MemoryPage) };						//������ ��������� �������� ������
	MemoryPage *base, *top, *reserved_page;								//������, ������� � ��������� �������
	FreeBlock *ftop;													//������� ���� � ������� ������������� ������
	size_t allocated_blocks, used_blocks;	
	bool force_page_write;												//��������� ������������� ������������� ������ � ������ ������ � ��������
public:
	PoolAllocator();
	PoolAllocator(const PoolAllocator&) = delete;
	PoolAllocator& operator=(const PoolAllocator&) = delete;
	PoolAllocator(PoolAllocator&& o) noexcept;
	PoolAllocator& operator=(PoolAllocator&& o) noexcept;
	inline ~PoolAllocator() noexcept { clear();  deallocate_page(base); }
public:									
	inline Ty* allocate() { ++used_blocks; return reinterpret_cast<Ty*>(allocate_block()); }	//���������� ��������� �� ������ ��� ������ ��������
	void deallocate(Ty* val);											//����������� ������
	void reserve(size_t	val_count);										//������������� �������� ������ � ��������� �������
	void clear();														//������������� ���������� ������, ����� ������ ��������
private:
	byte* allocate_block();												//���������� ��������� �� ��������� ��������� ����
	inline MemoryPage* allocate_page(size_t page_size)					//������� ��������
	{ byte* new_page{ new byte[header_size + page_size] }; return new (reinterpret_cast<MemoryPage*>(new_page)) MemoryPage(page_size, top); }
	inline void deallocate_page(MemoryPage* page) {  delete[] (reinterpret_cast<byte*>(page)); 	}	//������� ��������
	inline void make_free(Ty* ptr) { ftop = new (reinterpret_cast<FreeBlock*>(ptr)) FreeBlock(ftop); }	//������ ���� � ������ �������������
};

template <class Ty>
const double PoolAllocator<Ty>::reserve_multiplier{ 1 };
template <class Ty>
const size_t PoolAllocator<Ty>::min_allocated_blocks{ 1 };

template <class Ty>
PoolAllocator<Ty>::PoolAllocator()
	: base{ nullptr }, top{ base }, reserved_page{ nullptr }, ftop{ nullptr }, allocated_blocks{ 0 }, used_blocks{ 0 }, force_page_write{ false } {
	ALLOCATOR_VERIFY(sizeof(FreeBlock) <= block_size, "Size of value can't be less than pointer size (in bytes)");
}

template <class Ty>
void PoolAllocator<Ty>::clear() {
	MemoryPage *mpage;
	while (top)															//������� ��� ��������
	{
		mpage = top;
		top = top->prev;
		deallocate_page(mpage);
	}
	deallocate_page(reserved_page);										//������� ��������� ��������. ���� ���� reserved_page == nullptr, ����� delete ���������
	reserved_page = nullptr;											//�� �������� �������� ��������� - ������ �����������!
	ftop = nullptr;
	top = nullptr;
	base = nullptr;
	allocated_blocks = 0;
	used_blocks = 0;
}

template <class Ty>
PoolAllocator<Ty>::PoolAllocator(PoolAllocator&& o) noexcept
	: base{ o.base }, top{ o.top }, reserved_page{ o.reserved_page }, ftop{ o.ftop }, allocated_blocks{ o.allocated_blocks }, used_blocks{ o.used_blocks }, force_page_write{ false } {
	o.base = nullptr;
	o.top = nullptr;
	o.reserved_page = nullptr;
	o.ftop = nullptr;
	o.allocated_blocks = 0;
	o.used_blocks = 0;
}

template <class Ty>
PoolAllocator<Ty>& PoolAllocator<Ty>::operator=(PoolAllocator&& o) noexcept {
	if (this != &o)
	{
		base = o.base;
		o.base = nullptr;
		top = o.top;
		o.top = nullptr;
		reserved_page = o.reserved_page;
		o.reserved_page = nullptr;
		ftop = o.ftop;
		o.ftop = nullptr;
		allocated_blocks = o.allocated_blocks;
		o.allocated_blocks = 0;
		used_blocks = o.used_blocks;
		o.used_blocks = 0;
		force_page_write = o.force_page_write;
		o.force_page_write = false;
	}
	return *this;
}

template <class Ty>
void PoolAllocator<Ty>::deallocate(Ty* val) {
	if (reinterpret_cast<byte*>(val) == reinterpret_cast<byte*>(top) + header_size + top->offset - block_size)	//���� ���� - ��������� ������� ������� ��������
	{
		top->offset -= block_size;										//���� ���� ���� - ������ ������� �� ��������, �� ������ ��� ������� �� block_size ��� ��� ��������
		if (!top->offset)												//������������ ������ ������� ��������
		{
			MemoryPage* empty_page{ top };
			allocated_blocks -= empty_page->size/block_size;
			top = top->prev;
			deallocate_page(empty_page);
			if (reserved_page)
			{
				allocated_blocks -= reserved_page->size / block_size;	//�� �������� ��������������� ����� ���������� ������
				deallocate_page(reserved_page);							//����� �������� ������ ������� �������� ������������� ���������� ���������
				reserved_page = nullptr;
			}
			if (base == empty_page) {
				top = nullptr;
				base = nullptr;											//��� ������ 2.3 � ���� - ��� ������� ������ �������� base ���������� � ���������� ���������!
			}
		}
	}
	else {
		make_free(val);
	}	
	--used_blocks;
}

template <class Ty>
typename PoolAllocator<Ty>::byte* PoolAllocator<Ty>::allocate_block() {
	byte* block;
	if (!force_page_write && ftop)									//������������� ����� - � ����������
	{
		block = reinterpret_cast<byte*>(ftop);
		ftop = ftop->prev;
	}
	else
	{
		if (!top || top->offset == top->size)							
		{
			MemoryPage* new_page;
			if (reserved_page)										//���� ������� �������� ���������, ���������, ���������� �� ���������
			{
				new_page = reserved_page;							//����������? �������!
				reserved_page = nullptr;
			} 
			else
			{														//������������� ��������� ������ ������������ ������������� reserve_multiplier
				size_t new_blocks_count = min_allocated_blocks + allocated_blocks * reserve_multiplier;
				new_page = allocate_page(new_blocks_count * block_size);
				allocated_blocks += new_blocks_count;				//��������� �������� ���? ��� �, �������� �������� ������
				force_page_write = false;
			}
			new_page->prev = top;									
			top = new_page;											//��������� ��������� �� ������� ��������
			if (!base) {
				base = new_page;
			}
		}
		block = (reinterpret_cast<byte*>(top)) + header_size + top->offset;
		top->offset += block_size;									//������� ����� �� ������ �����
	}
	return block;
}

template <class Ty>
void PoolAllocator<Ty>::reserve(size_t val_count) {
	if (val_count > 0)
	{
		size_t new_blocks_count{ val_count - ((top->size - top->offset) / block_size) };	//��������� ���-�� ������, ������� ����� �������
		if (new_blocks_count > 0)
		{		
			if (!reserved_page || reserved_page->size/block_size < new_blocks_count)		//��������� �������� ��� ��� �� ������ ������ ����������?
			{	
				if (reserved_page)															//���� ��������� �������� ���-���� ����, �� ������������� �������...
				{
					allocated_blocks -= reserved_page->size;								//...������������ ������� ���������� ������ � ������� ��������� ��������
					deallocate_page(reserved_page);						
				}
				reserved_page = allocate_page(new_blocks_count * block_size);				//������� ����� ��������� ��������
				allocated_blocks += new_blocks_count;										//����������� ������� ���������� ������
			}
			if (top->offset == top->size)													//���� top ���������, ��������� �������� ���������� �������...
			{
				reserved_page->prev = top;
				top = reserved_page;
				reserved_page = nullptr;													//...� ��������� ���� ���������
			}
		}
		force_page_write = true;															//��������� ������ � �������� ������ ������������� ����� ������
	}
}
#endif	//PoolAllocator_H