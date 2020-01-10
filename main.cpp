#include <ganler_ref_cnter.hpp>

int main()
{
    {
        using namespace ganler::unsafe;
        ref_cnter cnter;
        ref_cnter cnter_;
        {
            auto fuck = cnter;
            cnter_ = cnter;
        }
    }

    {
        using namespace ganler::trivial;
        ref_cnter cnter;
        ref_cnter cnter_;
        { // Test passed.
            std::vector<std::future<void>> vec1(1600), vec2(1600), vec3(1600);
            for(auto&& x : vec1)
                x = std::async([&](){
                    const auto cnter_foo = cnter;
                    cnter_ = cnter;
                });
            for(auto&& x : vec2)
                x = std::async([&](){
                    const ref_cnter cnter_foo;
                    cnter = cnter_foo;
                });
            for(auto&& x : vec3)
                x = std::async([&](){
                    ref_cnter cnter_foo;
                    cnter = std::move(cnter_foo);
                });
        }
    }
    {
        using namespace ganler::opt;
        ref_cnter cnter;
        ref_cnter cnter_;
        { // Test passed.
            std::vector<std::future<void>> vec1(600), vec2(600), vec3(600);
            for(auto&& x : vec1)
                x = std::async([&](){
                    cnter = cnter_;  // cnter -> cnter_(local-2, global-1)
                });
            for(auto&& x : vec2)
                x = std::async([&](){
                    const ref_cnter cnter_foo;
                    cnter = cnter_foo; // Where cnter is going. // cnter -> cnter_foo(local-1, global-1)
                });
            for(auto&& x : vec3)
                x = std::async([&](){
                    const ref_cnter cnter_foo;
                    cnter = std::move(cnter_foo); // Where cnter is going. // cnter -> cnter_foo(local-1, global-1)
                });
        }
        // assigned to some cnter_foo in vec2/vec3.
    }
    std::cout << ">>> :) Current test passed !\n";
}