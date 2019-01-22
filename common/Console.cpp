/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Console.hpp>
#include <WCDB/CoreConst.h>
#include <WCDB/Error.hpp>
#include <WCDB/Notifier.hpp>
#include <WCDB/Version.h>
#include <iostream>

namespace WCDB {

Console* Console::shared()
{
    static Console* s_shared = new Console;
    return s_shared;
}

Console::Console() : m_debuggable(false)
{
    setLogger(Console::logger);
    setPrinter(Console::printer);
}

void Console::setDebuggable(bool debuggable)
{
    m_debuggable.store(debuggable);
}

bool Console::debuggable()
{
    return Console::shared()->isDebuggable();
}

bool Console::isDebuggable()
{
    return m_debuggable.load();
}

void Console::setLogger(const Logger& logger)
{
    if (logger != nullptr) {
        Notifier::shared()->setNotification(
        std::numeric_limits<int>::min(), NotifierLoggerName, logger);
    } else {
        Notifier::shared()->unsetNotification(NotifierLoggerName);
    }
}

void Console::setPrinter(const Printer& printer)
{
    m_printer = printer;
}

void Console::printer(const String& message)
{
    std::cout << message;
}

void Console::logger(const Error& error)
{
    if (error.level == Error::Level::Ignore) {
        return;
    }

    bool isDebuggable = debuggable();
    if (error.level == Error::Level::Debug && !isDebuggable) {
        return;
    }

    std::ostringstream stream;
    stream << "[" << Error::levelName(error.level) << ": ";
    stream << (int) error.code();
    stream << ", ";
    if (!error.message.empty()) {
        stream << error.message;
    } else {
        stream << error.codeName(error.code());
    }
    stream << "]";

    for (const auto& info : error.infos.getIntegers()) {
        stream << ", " << info.first << ": " << info.second;
    }
    for (const auto& info : error.infos.getStrings()) {
        if (!info.second.empty()) {
            stream << ", " << info.first << ": " << info.second;
        }
    }
    for (const auto& info : error.infos.getDoubles()) {
        stream << ", " << info.first << ": " << info.second;
    }
    stream << std::endl;

    if (!isDebuggable && error.level == Error::Level::Fatal) {
        stream << "Enable [debuggable] to debug." << std::endl;
    }
    // Is this hint important?
    //    else if (isDebuggable && error.level >= Error::Level::Error) {
    //        stream << "Set breakpoint at Console::breakpoint to debug." << std::endl;
    //    }

    Console::shared()->print(stream.str());

    if (isDebuggable && error.level >= Error::Level::Error) {
        breakpoint();
    }
}

void Console::breakpoint()
{
}

void Console::print(const String& message)
{
    if (m_printer != nullptr) {
        m_printer(message);
    }
}

#ifdef DEBUG
void Console::fatal(const String& message, const char* file, int line)
{
    Error error;
    error.setCode(Error::Code::Misuse, "Assertion");
    error.level = Error::Level::Fatal;
    error.message = message;
    if (file) {
        error.infos.set("File", file);
    }
    error.infos.set("Line", line);
    error.infos.set("Version", WCDB_VERSION);
    error.infos.set("BuildTime", WCDB_BUILD_TIME);
    error.infos.set("BuildTimestamp", WCDB_BUILD_TIMESTAMP);
    error.infos.set("CommitHash", WCDB_COMMIT_HASH);
    Notifier::shared()->notify(error);
}
#else  // DEBUG
void Console::fatal(const String& message)
{
    Error error;
    error.setCode(Error::Code::Misuse, "Assertion");
    error.level = Error::Level::Fatal;
    error.message = message;
    error.infos.set("Version", WCDB_VERSION);
    error.infos.set("BuildTime", WCDB_BUILD_TIME);
    error.infos.set("BuildTimestamp", WCDB_BUILD_TIMESTAMP);
    error.infos.set("CommitHash", WCDB_COMMIT_HASH);
    Notifier::shared()->notify(error);
}
#endif // DEBUG

} // namespace WCDB
