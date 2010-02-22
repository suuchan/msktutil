/*
 *----------------------------------------------------------------------------
 *
 * msktutil.h
 *
 * (C) 2004-2006 Dan Perry (dperry@pppl.gov)
 * (C) 2010 James Y Knight (foom@fuhm.net)
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *-----------------------------------------------------------------------------
 */
#include "msktutil.h"

KRB5Context::KRB5Context() {
    VERBOSE("Creating Kerberos Context");
    krb5_error_code ret = krb5_init_context(&m_context);
    if (ret)
        throw KRB5Exception("krb5_init_context", ret);
}

KRB5Context::~KRB5Context() {
    VERBOSE("Destroying Kerberos Context");
    krb5_free_context(m_context);
}

void KRB5Context::reload() {
    VERBOSE("Reloading Kerberos Context");
    krb5_free_context(m_context);
    krb5_error_code ret = krb5_init_context(&m_context);
    if (ret)
        throw KRB5Exception("krb5_init_context", ret);
}

void KRB5Keyblock::from_string(krb5_enctype enctype, std::string &password, std::string &salt) {
    krb5_data salt_data, pass_data;
    salt_data.data = const_cast<char *>(salt.c_str());
    salt_data.length = salt.length();

    pass_data.data = const_cast<char *>(password.c_str());
    pass_data.length = password.length();

    krb5_error_code ret = krb5_c_string_to_key(g_context.get(), enctype,
                                               &pass_data, &salt_data, &m_keyblock);
    if (ret)
        throw KRB5Exception("krb5_string_to_key", ret);
}


std::string KRB5Principal::name() {
    char *principal_string;
    krb5_error_code ret = krb5_unparse_name(g_context.get(), m_princ, &principal_string);
    if (ret)
        throw KRB5Exception("krb5_unparse_name", ret);

    std::string result(principal_string);

    krb5_free_unparsed_name(g_context.get(), principal_string);

    return result;
}

void KRB5Keytab::addEntry(KRB5Principal &princ, krb5_kvno kvno, KRB5Keyblock &keyblock) {
    krb5_keytab_entry entry;

    entry.principal = princ.get();
    entry.vno = kvno;
    entry.key = keyblock.get();
    krb5_error_code ret = krb5_kt_add_entry(g_context.get(), m_keytab, &entry);
    if (ret)
        throw KRB5Exception("krb5_kt_add_entry", ret);
}

void KRB5Keytab::removeEntry(KRB5Principal &princ, krb5_kvno kvno, krb5_enctype enctype) {
    krb5_keytab_entry entry;

    entry.principal = princ.get();
    entry.vno = kvno;
    entry.key.enctype = enctype;

    krb5_error_code ret = krb5_kt_remove_entry(g_context.get(), m_keytab, &entry);
    if (ret)
        throw KRB5Exception("krb5_kt_remove_entry", ret);
}

KRB5Keytab::cursor::cursor(KRB5Keytab &keytab) : m_keytab(keytab), m_cursor(), m_entry(), m_princ() {
    krb5_error_code ret = krb5_kt_start_seq_get(g_context.get(), m_keytab.m_keytab, &m_cursor);
    if (ret)
        throw KRB5Exception("krb5_kt_start_seq_get", ret);
}

KRB5Keytab::cursor::~cursor() {
    krb5_free_keytab_entry_contents(g_context.get(), &m_entry);
    memset(&m_entry, 0, sizeof(m_entry));
    // Tell m_princ to not free its contents!
    m_princ.reset_no_free(NULL);
    krb5_error_code ret = krb5_kt_end_seq_get(g_context.get(), m_keytab.m_keytab, &m_cursor);
    if (ret)
        // FIXME: shouldn't throw from destructor...
        throw KRB5Exception("krb5_kt_end_seq_get", ret);
}

bool KRB5Keytab::cursor::next() {
    krb5_free_keytab_entry_contents(g_context.get(), &m_entry);
    memset(&m_entry, 0, sizeof(m_entry));
    krb5_error_code ret = krb5_kt_next_entry(g_context.get(), m_keytab.m_keytab, &m_entry, &m_cursor);
    m_princ.reset_no_free(m_entry.principal);
    return ret == 0;
}

// GLOBAL:
KRB5Context g_context;