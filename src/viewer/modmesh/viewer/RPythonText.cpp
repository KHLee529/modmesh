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

#include <modmesh/viewer/base.hpp> // Must be the first include.
#include <modmesh/viewer/RPythonText.hpp>

#include <pybind11/embed.h>

namespace modmesh
{

RPythonText::RPythonText(
    QString const & title,
    QWidget * parent,
    Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    setUp();
}

void RPythonText::setUp()
{
    m_text = new QTextEdit();
    m_text->setFont(QFont("Courier New"));
    m_run = new QPushButton(QString("run"));

    m_layout = new QVBoxLayout();
    m_layout->addWidget(m_text);
    m_layout->addWidget(m_run);
    m_widget = new QWidget();
    m_widget->setLayout(m_layout);

    setWidget(m_widget);

    connect(m_run, &QPushButton::clicked, this, &RPythonText::runPythonCode);

    m_text->setPlainText(QString(R""""(# Sample input
import modmesh as mm
mh = mm.StaticMesh2d(nnode=4, nface=0, ncell=3)
mh.ndcrd.ndarray[:, :] = (0, 0), (-1, -1), (1, -1), (0, 1)
mh.cltpn.ndarray[:] = 4
mh.clnds.ndarray[:, :4] = (3, 0, 1, 2), (3, 0, 2, 3), (3, 0, 3, 1)
mh.build_interior()
mh.build_boundary()
mh.build_ghost()
mm.view.show(mh))""""));
}

void RPythonText::runPythonCode()
{
    namespace py = pybind11;

    std::string code = m_text->toPlainText().toStdString();
    try
    {
        py::exec(code);
    }
    catch (const py::error_already_set & e)
    {
        std::cerr << e.what() << std::endl;
    }
}

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4: