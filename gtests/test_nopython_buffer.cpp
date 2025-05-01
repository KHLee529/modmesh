#include <modmesh/buffer/buffer.hpp>

#include <gtest/gtest.h>

#ifdef Py_PYTHON_H
#error "Python.h should not be included."
#endif

TEST(ConcreteBuffer, iterator)
{
    using namespace modmesh;

    auto buffer = ConcreteBuffer::construct(10);
    int8_t i = 0;
    for (auto & it : *buffer)
    {
        it = i++;
    }

    i = 0;
    for (const auto it : *buffer)
    {
        EXPECT_EQ(it, i++);
    }
}

TEST(SimpleArray, construction)
{
    namespace mm = modmesh;
    mm::SimpleArray<double> arr_double(10);
    EXPECT_EQ(arr_double.nbody(), 10);
    mm::SimpleArray<int> arr_int(17);
    EXPECT_EQ(arr_int.nbody(), 17);
}

TEST(SimpleArray, minmaxsum)
{
    using namespace modmesh;

    SimpleArray<double> arr_double(small_vector<size_t>{10}, 0);
    EXPECT_EQ(arr_double.sum(), 0);
    EXPECT_EQ(arr_double.min(), 0);
    EXPECT_EQ(arr_double.max(), 0);
    arr_double.fill(3.14);
    EXPECT_EQ(arr_double.sum(), 3.14 * 10);
    EXPECT_EQ(arr_double.min(), 3.14);
    EXPECT_EQ(arr_double.max(), 3.14);
    arr_double(2) = -2.9;
    arr_double(4) = 12.7;
    EXPECT_EQ(arr_double.min(), -2.9);
    EXPECT_EQ(arr_double.max(), 12.7);

    SimpleArray<int> arr_int(small_vector<size_t>{3, 4}, -2);
    EXPECT_EQ(arr_int.sum(), -2 * 3 * 4);
    EXPECT_EQ(arr_int.min(), -2);
    EXPECT_EQ(arr_int.max(), -2);
    arr_int.fill(7);
    EXPECT_EQ(arr_int.sum(), 7 * 3 * 4);
    EXPECT_EQ(arr_int.min(), 7);
    EXPECT_EQ(arr_int.max(), 7);
    arr_int(1, 2) = -8;
    arr_int(2, 0) = 9;
    EXPECT_EQ(arr_int.min(), -8);
    EXPECT_EQ(arr_int.max(), 9);
}

TEST(SimpleArray, abs)
{
    using namespace modmesh;

    SimpleArray<double> arr(small_vector<size_t>{10}, -1.0);
    EXPECT_EQ(arr.sum(), -10.0);

    SimpleArray<double> brr = arr.abs();
    EXPECT_EQ(brr.sum(), 10.0);
}

TEST(SimpleArray, arithmetic_num)
{
    using namespace modmesh;

#define dst_init {2, 4, 6, 8, 10, 12, 14, 16, 18, 20}
#define src_init {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

    int add_res[] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 30};
    int sub_res[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int mul_res[] = {2, 8, 18, 32, 50, 72, 98, 128, 162, 200};
    int div_res[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

    {
        SimpleArray<int> src(src_init);
        SimpleArray<int> dst(dst_init);
        dst += src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), add_res[i]);
        }
    }
    {
        SimpleArray<int> src(src_init);
        SimpleArray<int> dst(dst_init);
        dst -= src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), sub_res[i]);
        }
    }
    {
        SimpleArray<int> src(src_init);
        SimpleArray<int> dst(dst_init);
        dst *= src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), mul_res[i]);
        }
    }
    {
        SimpleArray<int> src(src_init);
        SimpleArray<int> dst(dst_init);
        dst /= src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), div_res[i]);
        }
    }

#undef dst_init
#undef src_init
}

TEST(SimpleArray, arithmetic_bool)
{
    using namespace modmesh;

#define dst_init {true, true, true, false, false, false}
#define src_init {true, false, true, false, true, false}

    bool add_res[] = {true, true, true, false, true, false};
    bool mul_res[] = {true, false, true, false, false, false};

    {
        SimpleArray<bool> src(src_init);
        SimpleArray<bool> dst(dst_init);
        dst += src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), add_res[i]);
        }
    }
    {
        SimpleArray<bool> src(src_init);
        SimpleArray<bool> dst(dst_init);
        dst *= src;

        for (int i = 0; i < dst.size(); i++)
        {
            EXPECT_EQ(dst.data(i), mul_res[i]);
        }
    }
    {
        SimpleArray<bool> src(src_init);
        SimpleArray<bool> dst(dst_init);
        EXPECT_THROW({ dst -= src; }, std::runtime_error);
    }

#undef dst_init
#undef src_init
}

TEST(SimpleArray, iterator)
{
    using namespace modmesh;

    SimpleArray<double> arr(10);
    int8_t i = 0;
    for (auto & it : arr)
    {
        it = i++;
    }

    i = 0;
    for (const auto it : arr)
    {
        EXPECT_EQ(it, i++);
    }
}

TEST(SimpleArray_DataType, from_type)
{
    modmesh::DataType dt_double = modmesh::DataType::from<double>();
    EXPECT_EQ(dt_double.type(), modmesh::DataType::Float64);

    modmesh::DataType dt_int = modmesh::DataType::from<int>();
    EXPECT_EQ(dt_int.type(), modmesh::DataType::Int32);
}

TEST(SimpleArray_DataType, from_string)
{
    modmesh::DataType dt_double = modmesh::DataType("float64");
    EXPECT_EQ(dt_double.type(), modmesh::DataType::Float64);

    modmesh::DataType dt_bool = modmesh::DataType("bool");
    EXPECT_EQ(dt_bool.type(), modmesh::DataType::Bool);

    EXPECT_THROW(modmesh::DataType("float16"), std::invalid_argument); // float16 does not exist
    EXPECT_THROW(modmesh::DataType("bool8"), std::invalid_argument); // bool8 does not exist
}

TEST(BufferExpander, iterator)
{
    using namespace modmesh;

    auto buffer = BufferExpander::construct(10);
    int8_t i = 0;
    for (auto & it : *buffer)
    {
        it = i++;
    }

    i = 0;
    for (const auto it : *buffer)
    {
        EXPECT_EQ(it, i++);
    }
}

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
