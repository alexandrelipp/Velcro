//
// Created by alexa on 2022-04-20.
//

#include <catch2/catch_test_macros.hpp>

// CPP rule of 3: The Rule of Three states that if a type ever needs to have a user-defined copy constructor,
// copy assignment operator, or destructor, then it must have all three. These operations should all happened on the same members :
// this means a destructor  should never free a resource that was not created by the constructor


static int nConstruct = 0;
static int nMoves = 0;
static int nCopy = 0;
static int nDestruct = 0;

class A{
public:
    A() : s("test"), k(-1) { ++nConstruct; }
    ~A() { ++nDestruct; }
    A(const A& o) : s(o.s), k(o.k) { ++nCopy; }
    A(A&& o) noexcept :
            s(std::move(o.s)),       // explicit move of a member of class type
            k(std::exchange(o.k, 0)) // explicit move of a member of non-class type
    { ++nMoves; }

private:
    std::string s;
    int k;
};

TEST_CASE( "VectorEmplaceBack", "[Vector]" ) {
    std::vector<A> ok;
    ok.reserve(4);

    // emplace 4 elements
    ok.emplace_back();
    ok.emplace_back();
    ok.emplace_back();
    ok.emplace_back();

    // the elements should only be constructed
    REQUIRE(nConstruct == 4);
    REQUIRE(nMoves == 0);
    REQUIRE(nDestruct == 0);
    REQUIRE(nCopy == 0);

    // emplace one more, exceeding vector capacity
    ok.emplace_back();

    // the 4 older elements should have been moved + destructed
    REQUIRE(nMoves == 4);
    REQUIRE(nDestruct == 4);

    // one new element is constructed
    REQUIRE(nConstruct == 5);
    REQUIRE(nCopy == 0);
}
