#include <benchmark/benchmark.h>
#include <exception>
#include <cstring>
#include <cxxabi.h>
#include <cassert>
#include <functional>

#include "better_exception_ptr.h"

using namespace std::literals;

struct Foo {
    virtual ~Foo() {}
    int i = 1;
};
struct Bar : Foo, std::logic_error {
    Bar() : std::logic_error(""s) {}
    int j = 2;
};
struct Baz: Bar {};
using T = std::exception;

static void BM_throwCatchTrivial(benchmark::State& state) {
    auto shouldBeTrivial = [] {
        try {
            throw 0;
        } catch (int ex) {
            return ex;
        }
        return 1;
    };

    for (auto _ : state) {
        shouldBeTrivial();
    }
}
BENCHMARK(BM_throwCatchTrivial);
static void BM_throwCatch(benchmark::State& state) {
    const auto exp = std::make_exception_ptr(Baz());
    for (auto _ : state) {
        try {
            std::rethrow_exception(exp);
        } catch (Baz&ex) {

        }
    }
}
BENCHMARK(BM_throwCatch);

static void BM_EP_try_catch(benchmark::State& state) {
    const auto exp = std::make_exception_ptr(Baz());
    // Outside the benchmark section because real impl wouldn't be copying the exception_ptr.
    const auto exp2 = stdx::exception_ptr(exp);

    for (auto _ : state){
        auto ex = exp2.try_catch<Baz&>();
        assert(ex);
        assert(ex->i == 1);
        assert(ex->j == 2);
    }
}
BENCHMARK(BM_EP_try_catch);

static void BM_today_Lippincott(benchmark::State& state) {
    try {
        throw Baz();
    } catch (...) {
        for (auto _ : state){
            try {
                throw;
            } catch (Baz& ex) {
                benchmark::DoNotOptimize(ex.i);
            }
        }
    }
}
BENCHMARK(BM_today_Lippincott);

static void BM_EP_Lippincott(benchmark::State& state) {
    try {
        throw Baz();
    } catch (...) {
        for (auto _ : state){
            stdx::current_exception().handle_or_terminate(
                [&](Baz& ex) {
                    benchmark::DoNotOptimize(ex.i);
                }
            );
        }
    }
}
BENCHMARK(BM_EP_Lippincott);

BENCHMARK_MAIN();
