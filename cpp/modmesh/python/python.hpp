#pragma once

/*
 * Copyright (c) 2019, Yung-Yu Chen <yyc@solvcon.net>
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

#include <pybind11/pybind11.h> // Must be the first include.
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include <modmesh/modmesh.hpp>
#include <modmesh/python/common.hpp>

namespace modmesh
{

namespace python
{

#pragma GCC diagnostic push
// Suppress the warning "greater visibility than the type of its field"
#pragma GCC diagnostic ignored "-Wattributes"
/**
 * Take a pybind11 module and an initializing function and only run the
 * initializing function once.
 */
template <typename T>
class OneTimeInitializer
{

public:

    OneTimeInitializer(OneTimeInitializer const &) = delete;
    OneTimeInitializer(OneTimeInitializer &&) = delete;
    OneTimeInitializer & operator=(OneTimeInitializer const &) = delete;
    OneTimeInitializer & operator=(OneTimeInitializer &&) = delete;
    ~OneTimeInitializer() = default;

    static OneTimeInitializer<T> & me()
    {
        static OneTimeInitializer<T> instance;
        return instance;
    }

    OneTimeInitializer<T> & operator()(
        pybind11::module & mod, std::function<void(pybind11::module &)> const & initializer)
    {
        if (!initialized())
        {
            m_mod = &mod;
            m_initializer = initializer;
            m_initializer(*m_mod);
        }
        m_initialized = true;
        return *this;
    }

    pybind11::module const & mod() const { return *m_mod; }
    pybind11::module & mod() { return *m_mod; }

    bool initialized() const { return m_initialized && nullptr != m_mod; }

private:

    OneTimeInitializer() = default;

    bool m_initialized = false;
    pybind11::module * m_mod = nullptr;
    std::function<void(pybind11::module &)> m_initializer;

}; /* end class OneTimeInitializer */
#pragma GCC diagnostic pop

class MODMESH_PYTHON_WRAPPER_VISIBILITY Interpreter
{

public:

    static Interpreter & instance();

    Interpreter(Interpreter const &) = delete;
    Interpreter(Interpreter &&) = delete;
    Interpreter & operator=(Interpreter const &) = delete;
    Interpreter & operator=(Interpreter &&) = delete;
    ~Interpreter();

    void preload_module(std::string const & name);
    void preload_modules(std::vector<std::string> const & names);

private:

    Interpreter();
    void setup_path();

    pybind11::scoped_interpreter * m_interpreter = nullptr;

}; /* end class Interpreter */

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4: