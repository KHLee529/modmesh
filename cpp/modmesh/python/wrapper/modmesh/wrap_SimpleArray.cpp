/*
 * Copyright (c) 2022, Yung-Yu Chen <yyc@solvcon.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <modmesh/python/wrapper/modmesh/modmesh.hpp> // Must be the first include.
#include <modmesh/modmesh.hpp>

namespace modmesh
{

namespace python
{

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapSimpleArray
    : public WrapBase<WrapSimpleArray<T>, SimpleArray<T>>
{

    using root_base_type = WrapBase<WrapSimpleArray<T>, SimpleArray<T>>;
    using wrapped_type = typename root_base_type::wrapped_type;
    using shape_type = typename wrapped_type::shape_type;

    friend root_base_type;

    WrapSimpleArray(pybind11::module & mod, char const * pyname, char const * pydoc)
        : root_base_type(mod, pyname, pydoc, pybind11::buffer_protocol())
    {
        namespace py = pybind11;

        (*this)
            .def_timed(
                py::init(
                    [](py::object const & shape)
                    { return wrapped_type(make_shape(shape)); }),
                py::arg("shape"))
            .def(
                py::init(
                    [](py::array & arr_in)
                    {
                        if (!dtype_is_type<T>(arr_in))
                        {
                            throw std::runtime_error("dtype mismatch");
                        }
                        shape_type shape;
                        for (ssize_t i = 0; i < arr_in.ndim(); ++i)
                        {
                            shape.push_back(arr_in.shape(i));
                        }
                        std::shared_ptr<ConcreteBuffer> buffer = ConcreteBuffer::construct(
                            arr_in.nbytes(),
                            arr_in.mutable_data(),
                            std::make_unique<ConcreteBufferNdarrayRemover>(arr_in));
                        return wrapped_type(shape, buffer);
                    }),
                py::arg("array"))
            .def_buffer(
                [](wrapped_type & self)
                {
                    std::vector<size_t> stride;
                    for (size_t i : self.stride())
                    {
                        stride.push_back(i * sizeof(T));
                    }
                    return py::buffer_info(
                        self.data(), /* Pointer to buffer */
                        sizeof(T), /* Size of one scalar */
                        py::format_descriptor<T>::format(), /* Python struct-style format descriptor */
                        self.ndim(), /* Number of dimensions */
                        std::vector<size_t>(self.shape().begin(), self.shape().end()), /* Buffer dimensions */
                        stride /* Strides (in bytes) for each index */
                    );
                })
            .def_property_readonly(
                "ndarray",
                [](wrapped_type & self)
                { return to_ndarray(self); })
            .def_property_readonly(
                "is_from_python",
                [](wrapped_type const & self)
                {
                    return self.buffer().has_remover() && ConcreteBufferNdarrayRemover::is_same_type(self.buffer().get_remover());
                })
            .def_property_readonly("nbytes", &wrapped_type::nbytes)
            .def_property_readonly("size", &wrapped_type::size)
            .def_property_readonly("itemsize", &wrapped_type::itemsize)
            .def_property_readonly(
                "shape",
                [](wrapped_type const & self)
                {
                    py::tuple ret(self.shape().size());
                    for (size_t i = 0; i < self.shape().size(); ++i)
                    {
                        ret[i] = self.shape()[i];
                    }
                    return ret;
                })
            .def_property_readonly(
                "stride",
                [](wrapped_type const & self)
                {
                    py::tuple ret(self.stride().size());
                    for (size_t i = 0; i < self.stride().size(); ++i)
                    {
                        ret[i] = self.stride()[i];
                    }
                    return ret;
                })
            .def("__len__", &wrapped_type::size)
            .def(
                "__getitem__",
                [](wrapped_type const & self, ssize_t key)
                { return self.at(key); })
            .def(
                "__getitem__",
                [](wrapped_type const & self, std::vector<ssize_t> const & key)
                { return self.at(key); })
            .def("__setitem__", &setitem_parser)
            .def(
                "reshape",
                [](wrapped_type const & self, py::object const & shape)
                { return self.reshape(make_shape(shape)); })
            .def_property_readonly("has_ghost", &wrapped_type::has_ghost)
            .def_property("nghost", &wrapped_type::nghost, &wrapped_type::set_nghost)
            .def_property_readonly("nbody", &wrapped_type::nbody)
            //
            ;
    }

    static void setitem_parser(wrapped_type & arr_out, pybind11::args & args)
    {
        namespace py = pybind11;

        if (args.size() == 2)
        {
            // sarr[K] = V
            if (py::isinstance<py::int_>(args[0]) &&
                (py::isinstance<py::bool_>(args[1]) || py::isinstance<py::int_>(args[1]) ||
                 py::isinstance<py::float_>(args[1])))
            {
                const auto key = args[0].cast<ssize_t>();

                arr_out.at(key) = args[1].cast<T>();
                return;
            }
            // sarr[K1, K2, K3] = V
            if (py::isinstance<py::tuple>(args[0]) &&
                (py::isinstance<py::bool_>(args[1]) || py::isinstance<py::int_>(args[1]) ||
                 py::isinstance<py::float_>(args[1])))
            {
                const auto key = args[0].cast<std::vector<ssize_t>>();

                arr_out.at(key) = args[1].cast<T>();
                return;
            }
            // multi-dimension with slice and ellipsis
            // sarr[slice, slice, ellipsis] = ndarr
            if (py::isinstance<py::tuple>(args[0]) && py::isinstance<py::array>(args[1]))
            {
                const auto tuple = args[0].cast<py::tuple>();
                const auto arr_in = args[1].cast<py::array>();

                auto slices = make_default_slice(arr_out);
                process_slice(tuple, slices, arr_out.ndim());

                broadcast_array_using_slice(arr_out, slices, arr_in);
                return;
            }
            // one-dimension with slice
            // sarr[slice] = ndarr
            if (py::isinstance<py::slice>(args[0]) && py::isinstance<py::array>(args[1]))
            {
                const auto slice_in = args[0].cast<py::slice>();
                const auto arr_in = args[1].cast<py::array>();

                auto slices = make_default_slice(arr_out);
                copy_slice(slices[0], slice_in);

                broadcast_array_using_slice(arr_out, slices, arr_in);
                return;
            }
            // sarr[ellipsis] = ndarr
            if (py::isinstance<py::ellipsis>(args[0]) && py::isinstance<py::array>(args[1]))
            {
                const auto arr_in = args[1].cast<py::array>();

                broadcast_array_using_ellipsis(arr_out, arr_in);
                return;
            }
        }
        throw std::runtime_error("unsupported operation.");
    }

    static void copy_slice(small_vector<int> & slice_out, pybind11::slice const & slice_in)
    {
        auto start = std::string(pybind11::str(slice_in.attr("start")));
        auto stop = std::string(pybind11::str(slice_in.attr("stop")));
        auto step = std::string(pybind11::str(slice_in.attr("step")));

        slice_out[0] = start == "None" ? slice_out[0] : std::stoi(start);
        slice_out[1] = stop == "None" ? slice_out[1] : std::stoi(stop);
        slice_out[2] = step == "None" ? slice_out[2] : std::stoi(step);
    }

    static std::vector<small_vector<int>> make_default_slice(wrapped_type const & arr)
    {
        std::vector<small_vector<int>> slices;
        slices.reserve(arr.ndim());
        for (size_t i = 0; i < arr.ndim(); ++i)
        {
            small_vector<int> default_slice(3);
            default_slice[0] = 0; // start
            default_slice[1] = arr.shape(i); // stop
            default_slice[2] = 1; // step
            slices.push_back(std::move(default_slice));
        }
        return slices;
    }

    static void process_slice(pybind11::tuple const & tuple,
                              std::vector<small_vector<int>> & slices,
                              size_t ndim)
    {
        namespace py = pybind11;

        // copy slices from the front untill an ellipsis
        bool ellipsis_flag = false;
        for (auto it = tuple.begin(); it != tuple.end(); it++)
        {
            if (py::isinstance<py::ellipsis>(*it))
            {
                // stop here and iterator the tuple from back later
                ellipsis_flag = true;
                break;
            }

            auto & slice_out = slices[it - tuple.begin()];
            const auto slice_in = (*it).cast<py::slice>();

            copy_slice(slice_out, slice_in);
        }

        // copy slices from the back until an ellipsis
        if (ellipsis_flag)
        {
            for (size_t size = 0; size < tuple.size(); size++)
            {
                auto it = tuple.end() - size - 1;

                if (py::isinstance<py::ellipsis>(*it))
                {
                    break;
                }
                auto & slice_out = slices[ndim - size - 1];
                const auto slice_in = (*it).cast<py::slice>();

                copy_slice(slice_out, slice_in);
            }
        }
    }

    static void slice_syntax_check(pybind11::tuple const & tuple, size_t ndim)
    {
        namespace py = pybind11;

        size_t ellipsis_cnt = 0;
        size_t slice_cnt = 0;

        for (auto it = tuple.begin(); it != tuple.end(); it++)
        {
            if (py::isinstance<py::ellipsis>(*it))
            {
                ellipsis_cnt += 1;
            }
            else if (py::isinstance<py::slice>(*it))
            {
                slice_cnt += 1;
            }
            else
            {
                throw std::runtime_error("unsupported operation.");
            }
        }

        if (ellipsis_cnt + slice_cnt > ndim)
        {
            throw std::runtime_error("syntax error. dimensions mismatches");
        }

        if (ellipsis_cnt > 1)
        {
            throw std::runtime_error("syntax error. no more than one ellipsis.");
        }
    }

    static void broadcast_array_using_slice(wrapped_type & arr_out,
                                            std::vector<small_vector<int>> const & slices,
                                            pybind11::array const & arr_in)
    {
        TypeBroadCast::check_shape(arr_out, slices, arr_in);

        size_t nghost = 0;

        if (arr_out.has_ghost())
        {
            nghost = arr_out.nghost();
            arr_out.set_nghost(0);
        }

        TypeBroadCast::broadcast(arr_out, slices, arr_in);

        if (nghost != 0)
        {
            arr_out.set_nghost(nghost);
        }
    }

    struct TypeBroadCast
    {
        static void check_shape(wrapped_type const & arr_out,
                                std::vector<small_vector<int>> const & slices,
                                pybind11::array const & arr_in)
        {
            shape_type right_shape(arr_in.ndim());
            for (pybind11::ssize_t i = 0; i < arr_in.ndim(); i++)
            {
                right_shape[i] = arr_in.shape(i);
            }

            shape_type left_shape(arr_out.ndim());
            // todo: range check
            for (size_t i = 0; i < arr_out.ndim(); i++)
            {
                const auto & slice = slices[i];
                if ((slice[1] - slice[0]) % slice[2] == 0)
                {
                    left_shape[i] = (slice[1] - slice[0]) / slice[2];
                }
                else
                {
                    left_shape[i] = (slice[1] - slice[0]) / slice[2] + 1;
                }
            }

            if (arr_out.ndim() != static_cast<size_t>(arr_in.ndim()))
            {
                throw_shape_error(left_shape, right_shape);
            }

            for (size_t i = 0; i < left_shape.size(); ++i)
            {
                if (left_shape[i] != static_cast<size_t>(right_shape[i]))
                {
                    throw_shape_error(left_shape, right_shape);
                }
            }
        }

        static void
        broadcast(wrapped_type & arr_out,
                  std::vector<small_vector<int>> const & slices,
                  pybind11::array const & arr_in)
        {
            if (dtype_is_type<bool>(arr_in))
            {
                TypeBroadCastImpl<bool>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<int8_t>(arr_in))
            {
                TypeBroadCastImpl<int8_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<int16_t>(arr_in))
            {
                TypeBroadCastImpl<int16_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<int32_t>(arr_in))
            {
                TypeBroadCastImpl<int32_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<int64_t>(arr_in))
            {
                TypeBroadCastImpl<int64_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<uint32_t>(arr_in))
            {
                TypeBroadCastImpl<uint32_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<uint16_t>(arr_in))
            {
                TypeBroadCastImpl<uint16_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<uint32_t>(arr_in))
            {
                TypeBroadCastImpl<uint32_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<uint64_t>(arr_in))
            {
                TypeBroadCastImpl<uint64_t>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<float>(arr_in))
            {
                TypeBroadCastImpl<float>::broadcast(arr_out, slices, arr_in);
            }
            else if (dtype_is_type<double>(arr_in))
            {
                TypeBroadCastImpl<double>::broadcast(arr_out, slices, arr_in);
            }
            else
            {
                throw std::runtime_error("input array data type not support!");
            }
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        static void throw_shape_error(shape_type const & left_shape, shape_type const & right_shape)
        {
            std::ostringstream msg;
            msg << "Broadcast input array from shape(";
            for (size_t i = 0; i < right_shape.size(); ++i)
            {
                msg << right_shape[i];
                if (i != right_shape.size() - 1)
                {
                    msg << ", ";
                }
            }
            msg << ") into shape(";
            for (size_t i = 0; i < left_shape.size(); ++i)
            {
                msg << left_shape[i];
                if (i != left_shape.size() - 1)
                {
                    msg << ", ";
                }
            }
            msg << ")";

            throw std::runtime_error(msg.str());
        };
    };

    template <typename D /* for destination type */>
    struct TypeBroadCastImpl
    {
        static void broadcast(wrapped_type & arr_out,
                              std::vector<small_vector<int>> const & slices,
                              pybind11::array const & arr_in)
        {
            using out_type = typename std::remove_reference<decltype(arr_out[0])>::type;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto * arr_new = reinterpret_cast<pybind11::array_t<D> const *>(&arr_in);

            shape_type left_shape(arr_out.ndim());
            for (size_t i = 0; i < arr_out.ndim(); i++)
            {
                const auto & slice = slices[i];
                if ((slice[1] - slice[0]) % slice[2] == 0)
                {
                    left_shape[i] = (slice[1] - slice[0]) / slice[2];
                }
                else
                {
                    left_shape[i] = (slice[1] - slice[0]) / slice[2] + 1;
                }
            }

            shape_type sidx_init(arr_out.ndim());

            for (size_t i = 0; i < arr_out.ndim(); ++i)
            {
                sidx_init[i] = 0;
            }

            std::function<void(shape_type, int)> copy_idx;
            copy_idx = [&](shape_type sidx, int dim)
            {
                if (dim < 0)
                {
                    return;
                }

                for (size_t i = 0; i < left_shape[dim]; ++i)
                {
                    sidx[dim] = i;

                    size_t offset_in = 0;
                    for (pybind11::ssize_t it = 0; it < arr_in.ndim(); ++it)
                    {
                        offset_in += arr_in.strides(it) / arr_in.itemsize() * sidx[it];
                    }
                    const D * ptr_in = arr_new->data() + offset_in;

                    size_t offset_out = 0;
                    for (size_t it = 0; it < arr_out.ndim(); ++it)
                    {
                        auto step = slices[it][2];
                        offset_out += arr_out.stride(it) * sidx[it] * step;
                    }

                    // NOLINTNEXTLINE(bugprone-signed-char-misuse, cert-str34-c)
                    arr_out.at(offset_out) = static_cast<out_type>(*ptr_in);
                    // recursion here
                    copy_idx(sidx, dim - 1);
                }
            };

            copy_idx(sidx_init, arr_out.ndim() - 1);
        }
    };

    static void
    broadcast_array_using_ellipsis(wrapped_type & arr_out, pybind11::array const & arr_in)
    {
        auto slices = make_default_slice(arr_out);

        TypeBroadCast::check_shape(arr_out, slices, arr_in);

        size_t nghost = 0;

        if (arr_out.has_ghost())
        {
            nghost = arr_out.nghost();
            arr_out.set_nghost(0);
        }

        TypeBroadCast::broadcast(arr_out, slices, arr_in);

        if (nghost != 0)
        {
            arr_out.set_nghost(nghost);
        }
    }

    static shape_type make_shape(pybind11::object const & shape_in)
    {
        namespace py = pybind11; // NOLINT(misc-unused-alias-decls)
        shape_type shape;
        try
        {
            shape.push_back(shape_in.cast<size_t>());
        }
        catch (const py::cast_error &)
        {
            shape = shape_in.cast<std::vector<size_t>>();
        }
        return shape;
    }

}; /* end class WrapSimpleArray */

void wrap_SimpleArray(pybind11::module & mod)
{
    WrapSimpleArray<bool>::commit(mod, "SimpleArrayBool", "SimpleArrayBool");
    WrapSimpleArray<int8_t>::commit(mod, "SimpleArrayInt8", "SimpleArrayInt8");
    WrapSimpleArray<int16_t>::commit(mod, "SimpleArrayInt16", "SimpleArrayInt16");
    WrapSimpleArray<int32_t>::commit(mod, "SimpleArrayInt32", "SimpleArrayInt32");
    WrapSimpleArray<int64_t>::commit(mod, "SimpleArrayInt64", "SimpleArrayInt64");
    WrapSimpleArray<uint8_t>::commit(mod, "SimpleArrayUint8", "SimpleArrayUint8");
    WrapSimpleArray<uint16_t>::commit(mod, "SimpleArrayUint16", "SimpleArrayUint16");
    WrapSimpleArray<uint32_t>::commit(mod, "SimpleArrayUint32", "SimpleArrayUint32");
    WrapSimpleArray<uint64_t>::commit(mod, "SimpleArrayUint64", "SimpleArrayUint64");
    WrapSimpleArray<float>::commit(mod, "SimpleArrayFloat32", "SimpleArrayFloat32");
    WrapSimpleArray<double>::commit(mod, "SimpleArrayFloat64", "SimpleArrayFloat64");
}

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
