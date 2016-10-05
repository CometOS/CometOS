/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @author Stefan Unterschuetz
 */


#ifndef TUPLE_H_
#define TUPLE_H_

class NonTypeTupleEntry {
};


/**Template class for Tuples of a maximum length of 4 items. In the future
 * this can be exchanged by an implementation using variadic templates of
 * C++11.
 */
template<class C1 = NonTypeTupleEntry, class C2 = NonTypeTupleEntry, class C3 = NonTypeTupleEntry,
		class C4 = NonTypeTupleEntry, class C5 = NonTypeTupleEntry, class C6 = NonTypeTupleEntry>
class Tuple {
};


template<class C1>
class Tuple<C1, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> {
public:
	Tuple() {
	}

	Tuple(const C1& first) :
			first(first) {
	}

	const Tuple& operator=(const Tuple& val) {
		first = val.first;
		return *this;
	}

	bool operator==(const Tuple& val) {
		return (first == val.first);
	}

	C1 first;
};

template<class C1, class C2>
class Tuple<C1, C2, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> : public Tuple<C1, NonTypeTupleEntry,
		NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> {
public:
	Tuple() {
	}
	Tuple(const C1& first, const C2& second) :
			Tuple<C1, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>(first),
			second(second) {
	}
	const Tuple& operator=(const Tuple& val) {
		Tuple<C1, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator=(val);
		second = val.second;
		return *this;
	}

	bool operator==(const Tuple& val) {
		return (second == val.second)
				&& Tuple<C1, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator==(
						val);
	}

	C2 second;
};

template<class C1, class C2, class C3>
class Tuple<C1, C2, C3, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> : public Tuple<C1, C2, NonTypeTupleEntry,
		NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> {
public:
	Tuple() {
	}
	Tuple(const C1& first, const C2& second, const C3& third) :
			Tuple<C1, C2, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>(first, second),
			third(third) {
	}

	const Tuple& operator=(const Tuple& val) {
		Tuple<C1, C2, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator=(val);
		third = val.third;
		return *this;
	}

	bool operator==(const Tuple& val) {
		return (third == val.third)
				&& Tuple<C1, C2, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator==(val);
	}

	C3 third;
};


template<class C1, class C2, class C3, class C4>
class Tuple<C1, C2, C3, C4, NonTypeTupleEntry, NonTypeTupleEntry> : public Tuple<C1, C2, C3,
        NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry> {
public:
    Tuple() {
    }
    Tuple(const C1& first, const C2& second, const C3& third, const C4& fourth) :
            Tuple<C1, C2, C3, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>(first, second, third),
            fourth(fourth) {
    }

    const Tuple& operator=(const Tuple& val) {
        Tuple<C1, C2, C3, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator=(val);
        fourth = val.fourth;
        return *this;
    }

    bool operator==(const Tuple& val) {
        return (fourth == val.forth)
                && Tuple<C1, C2, C3, NonTypeTupleEntry, NonTypeTupleEntry, NonTypeTupleEntry>::operator==(val);
    }

    C4 fourth;
};


template<class C1, class C2, class C3, class C4, class C5>
class Tuple<C1, C2, C3, C4, C5, NonTypeTupleEntry> : public Tuple<C1, C2, C3,
        C4, NonTypeTupleEntry, NonTypeTupleEntry> {
public:
    Tuple() {
    }
    Tuple(const C1& first, const C2& second, const C3& third, const C4& fourth, const C5& fifth) :
            Tuple<C1, C2, C3, C4, NonTypeTupleEntry, NonTypeTupleEntry>(first, second, third, fourth),
            fifth(fifth) {
    }

    const Tuple& operator=(const Tuple& val) {
        Tuple<C1, C2, C3, C4, NonTypeTupleEntry, NonTypeTupleEntry>::operator=(val);
        fifth = val.fifth;
        return *this;
    }

    bool operator==(const Tuple& val) {
        return (fifth == val.fifth)
                && Tuple<C1, C2, C3, C4, NonTypeTupleEntry, NonTypeTupleEntry>::operator==(val);
    }

    C5 fifth;
};

template<int I, class... Cs>
class TupleGetter
{
};

template<class C1, class... OCs>
class TupleGetter<0, C1, OCs...>
{
public:
	static C1 get(Tuple<C1,OCs...> t)
	{
		return t.first;
	}
};

template<class C1, class C2, class... OCs>
class TupleGetter<1, C1, C2, OCs...>
{
public:
	static C2 get(Tuple<C1,C2,OCs...> t)
	{
		return t.second;
	}
};

template<class C1, class C2, class C3, class... OCs>
class TupleGetter<2, C1, C2, C3, OCs...>
{
public:
	static C3 get(Tuple<C1,C2,C3,OCs...> t)
	{
		return t.third;
	}
};


template<class C1, class C2, class C3, class C4, class... OCs>
class TupleGetter<3, C1, C2, C3, C4, OCs...>
{
public:
    static C4 get(Tuple<C1,C2,C3,C4,OCs...> t)
    {
        return t.fourth;
    }
};

template<class C1, class C2, class C3, class C4, class C5, class... OCs>
class TupleGetter<4, C1, C2, C3, C4, C5, OCs...>
{
public:
    static C5 get(Tuple<C1,C2,C3,C4,C5,OCs...> t)
    {
        return t.fifth;
    }
};

#endif /* TUPLE_H_ */
