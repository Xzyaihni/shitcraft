#ifndef YAN_CMAP_H
#define YAN_CMAP_H

#include <iterator>

#include <ythreads.h>

#include "types.h"
#include "cmodel.h"

class world_generator;

namespace cmap
{
	typedef std::vector<full_chunk> container_type;
	typedef std::vector<full_chunk*> ref_container_type;


	class controller;

	class storage
	{
	public:
		storage();
		storage(controller* owner, world_generator* generator, const int size);

		storage(const storage&);
		storage(storage&&) noexcept;
		storage& operator=(const storage&);
		storage& operator=(storage&&) noexcept;

		void generate_chunk(const vec3d<int> pos);

		void remove_chunk(container_type::iterator chunk);
		void remove_chunk(full_chunk& chunk);

		void clear() noexcept;

		container_type chunks;
		ref_container_type processed_chunks;

		mutable std::mutex chunk_gen_mtx;

	private:
		void copy_members(const storage&);
		void move_members(storage&&) noexcept;

		void remove_chunk(full_chunk& chunk, const int index);

		int _chunks_amount;

		std::vector<int> _open_spots;

		controller* _owner = nullptr;
		world_generator* _generator = nullptr;
	};

	class controller : public chunk_observer
	{
	public:
		struct iterator
		{
			using iterator_content = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = full_chunk*;
			using pointer = value_type*;
			using reference = full_chunk&;

			iterator(const value_type* end, pointer p);

			reference operator*() const;
			value_type operator->();

			iterator& operator++();
			iterator operator++(int);

			friend bool operator==(const iterator& a, const iterator& b);
			friend bool operator!=(const iterator& a, const iterator& b);

		private:
			pointer _ptr;
			const value_type* _end_ptr;
		};

		struct const_iterator
		{
			using iterator_content = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = const full_chunk*;
			using pointer = const value_type*;
			using reference = const full_chunk&;

			const_iterator(pointer end, pointer p);

			reference operator*() const;
			value_type operator->() const;

			const_iterator& operator++();
			const_iterator operator++(int);

			friend bool operator==(const const_iterator& a, const const_iterator& b);
			friend bool operator!=(const const_iterator& a, const const_iterator& b);

		private:
			pointer _ptr;
			pointer _end_ptr;
		};

		controller();
		controller(world_generator* generator, const int render_size, const vec3d<int> center_pos);

		controller(const controller&);
		controller& operator=(const controller&&);

		void update() noexcept;
		void update_center(const vec3d<int> pos);

		void block_notify(const vec3d<int> chunk, const vec3d<int> pos);

		full_chunk& at(const vec3d<int> pos);
		const full_chunk& at(const vec3d<int> pos) const;

		vec3d<int> difference(const vec3d<int> pos) const noexcept;

		bool in_bounds(const vec3d<int> pos) const noexcept;
		bool in_local_bounds(const vec3d<int> rel_pos) const noexcept;
		bool contains(const vec3d<int> pos) const noexcept;
		bool contains_local(const vec3d<int> rel_pos) const noexcept;

		iterator find(const vec3d<int> pos) noexcept;
		const_iterator find(const vec3d<int> pos) const noexcept;

		iterator begin() noexcept;
		const_iterator cbegin() const noexcept;
		const_iterator begin() const noexcept;
		iterator end() noexcept;
		const_iterator cend() const noexcept;
		const_iterator end() const noexcept;

		void clear() noexcept;

	private:
		void connect_processed() noexcept;

		void generate_missing();

		bool reassign_chunks(const vec3d<int> pos) noexcept;
		bool squares_overlap(const vec3d<int> pos) const noexcept;

		void move_chunk(const vec3d<int> rel_pos, const vec3d<int> offset) noexcept;

		void update_chunks(const vec3d<int> pos, const world_types::wall_states chunks) noexcept;
		void update_chunk(const vec3d<int> pos) noexcept;

		void update_walls(const vec3d<int> pos, const world_types::wall_states walls) noexcept;
		void update_wall(const vec3d<int> pos, const ytype::direction wall) noexcept;
		void update_side(const int index, const int side_index, const ytype::direction wall) noexcept;

		bool exists(const vec3d<int> pos) const noexcept;
		bool exists_local(const vec3d<int> rel_pos) const noexcept;
		bool exists(const int index) const noexcept;

		int index_chunk(const vec3d<int> pos) const noexcept;
		int index_local_chunk(const vec3d<int> rel_pos) const noexcept;
		vec3d<int> index_position(const int index) const noexcept;

		vec3d<int> position_global(const vec3d<int> rel_pos) const noexcept;
		vec3d<int> position_local(const vec3d<int> pos) const noexcept;

		void generate_all() noexcept;
		void generate_pool() noexcept;

		vec3d<int> _center_pos;

		int _render_size;
		int _row_size;
		int _chunks_amount;

		world_generator* _generator = nullptr;

		storage _chunks;
		std::vector<full_chunk*> _chunks_map;
		std::vector<bool> _status_flags;

		typedef ythreads::pool<void (storage::*)(const vec3d<int>),
			vec3d<int>, storage*> cgen_pool_type;
		std::unique_ptr<cgen_pool_type> _chunk_gen_pool;
	};
};

#endif
