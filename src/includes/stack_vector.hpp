#include <vector>


template<typename T, int N = ((sizeof(std::vector<T>) + sizeof(T) - 1) / sizeof(T))>
class stack_vector {
// ------------------------------------[ Properties ] --------------------------------------- //
public:
	using element_type = T;
	static constexpr int stack_capacity = N;
	
private:
	int count = 0;
	union {
		alignas(T[N])
		uint8_t arr[sizeof(T) * N];
		
		alignas(std::vector<T>)
		uint8_t vec[sizeof(std::vector<T>)];
	} memory;
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
private:
	inline T* arr(){
		return reinterpret_cast<T*>(memory.arr);
	}
	inline std::vector<T>& vec(){
		return reinterpret_cast<std::vector<T>&>(memory.vec);
	}
	
// ---------------------------------- [ Constructors ] -------------------------------------- //
public:
	stack_vector() = default;
	stack_vector(const stack_vector&) = delete;
	
	stack_vector(stack_vector&& o) : count{o.count} {
		if (count <= stack_capacity){
			T* src = o.arr();
			T* dst = this->arr();
			
			for (int i = 0 ; i < count ; i++){
				new (&dst[i]) T(std::move(src[i]));
				src[i].~T();
			}
			
		} else {
			std::vector<T>& src = o.vec();
			new (&this->vec()) std::vector<T>(std::move(src));
			src.~vector();
		}
		o.count = 0;
	}
	
	~stack_vector(){
		if (count <= stack_capacity){
			T* a = arr();
			for (int i = 0 ; i < count ; i++)
				a[i].~T();
		} else {
			vec().~vector();
		}
	}
	
// ----------------------------------- [ Functions ] ---------------------------------------- //
public:
	int size() const {
		return count;
	}
	
	T* begin(){
		return &this->operator[](0);
	}
	T* end(){
		return &this->operator[](0) + count;
	}
	
	const T* begin() const {
		return &this->operator[](0);
	}
	const T* end() const {
		return &this->operator[](0) + count;
	}
	
public:
	template<typename ...U>
	T& emplace_back(U&& ...el){
		// Append to array
		if (count < N){
			T* p = arr();
			new (p + count) T(std::forward<U>(el)...);
			return p[count++];
		}
		
		// Convert to vector
		if (count == N){
			std::vector<T> v;
			v.reserve(N*2);
			
			T* a = arr();
			for (int i = 0 ; i < count ; i++){
				v.emplace_back(std::move(a[i]));
				a[i].~T();
			}
			
			new (&vec()) std::vector<T>(std::move(v));
		}
		
		// Append to vector
		T& r = vec().emplace_back(std::forward<U>(el)...);
		count++;
		return r;
	}
	
// ----------------------------------- [ Operators ] ---------------------------------------- //
public:
	T& operator[](int i){
		if (count <= N)
			return arr()[i];
		else
			return vec()[i];
	}
	
	const T& operator[](int i) const {
		if (count <= N)
			return arr()[i];
		else
			return vec()[i];
	}
	
// ------------------------------------------------------------------------------------------ //
};