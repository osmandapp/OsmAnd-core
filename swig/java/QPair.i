%{
#include <QPair>
%}

// exported class

template<class T1, class T2> class QPair {
	public:
	QPair();
	QPair(const T1 &, const T2 &);
	%extend {
		const T1& getFirst() {
			return self->first;
		}
		const T2& getSecond() {
			return self->second;
		}
	}
};
