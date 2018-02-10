/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HISTORYSQL_H__
#define __HISTORYSQL_H__

#define SQL_STMT_VACUUM	"VACUUM"

#define SQL_STMT_PRAGMA_CASCADE "PRAGMA foreign_keys = ON;"

#define SQL_STMT_CREATE_TABLE_CONVERSATIONS "\
CREATE TABLE IF NOT EXISTS conversations \
( \
	id INTEGER PRIMARY KEY, \
	pluginId INTEGER NOT NULL, \
	contactId TEXT, \
	contactName TEXT, \
	userId TEXT NOT NULL, \
	flags INTEGER NOT NULL \
);"

#define SQL_STMT_CREATE_TABLE_MESSAGES "\
CREATE TABLE IF NOT EXISTS messages( \
	id INTEGER PRIMARY KEY, \
	conversationId INTEGER NOT NULL, \
	contactId TEXT, \
	contactName TEXT, \
	aTimestamp INTEGER NOT NULL, \
	flags INTEGER NOT NULL, \
	content TEXT NOT NULL, \
	FOREIGN KEY(conversationId) REFERENCES conversations(id) ON DELETE CASCADE \
);"

#define SQL_STMT_CREATE_INDEX_MESSAGES_CONVERSATIONS "CREATE INDEX IF NOT EXISTS conversationsIndex ON messages(conversationId);"

#define SQL_STMT_INSERT_CONVERSATION "INSERT INTO conversations VALUES(NULL, ?, ?, ?, ?, ?);"

#define SQL_STMT_INSERT_MESSAGE "INSERT INTO messages VALUES(NULL, ?, ?, ?, ?, ?, ?);"

#define SQL_STMT_SELECT_LAST_MESSAGES "\
SELECT flags, aTimestamp, contactId, content FROM \
(SELECT m.id, m.flags, m.aTimestamp, c.contactId, m.content FROM messages as m \
LEFT JOIN conversations as c ON m.conversationId = c.id \
WHERE c.pluginId = ?1 AND (c.contactId = ?2 OR m.contactId = ?2) \
ORDER BY m.id DESC LIMIT ?3) as w ORDER BY w.id ASC;"

#define SQL_STMT_SELECT_LAST_CONVERSATION_MESSAGES "\
SELECT flags, aTimestamp, contactId, content FROM messages as m \
WHERE m.conversationId = (SELECT c.id FROM conversations as c WHERE c.pluginId = ?1 AND c.contactId = ?2 ORDER BY c.id DESC LIMIT 1) \
ORDER BY m.id ASC;"

#define SQL_STMT_SELECT_LAST_MESSAGES_TIME "\
SELECT m.flags, m.aTimestamp, m.contactId, m.content FROM messages as m \
JOIN conversations as c ON m.conversationId = c.id \
WHERE c.pluginId = ?1 AND (c.contactId = ?2 OR m.contactId = ?2) AND m.aTimestamp >= ?3 \
ORDER BY m.id ASC;"

#define SQL_STMT_SELECT_LAST_CONVERSATION "\
SELECT c.id, m.aTimestamp FROM conversations as c \
JOIN messages as m ON (c.id = m.conversationId) \
WHERE c.pluginId = ?1 AND (c.contactId = ?2 OR m.contactId = ?2) \
ORDER BY c.id DESC LIMIT 1;"

#define SQL_STMT_SELECT_CONTACTS "\
SELECT c.contactId, c.contactName, c.pluginId \
FROM conversations as c \
GROUP BY c.contactId, c.pluginId;"

#define SQL_STMT_SELECT_CONVERSATIONS "\
SELECT c.id, c.contactId, c.contactName, c.userId, c.flags, MIN(m.aTimestamp), MAX(m.aTimestamp), c.pluginId \
FROM conversations as c JOIN messages as m ON (c.id = m.conversationId) \
WHERE c.pluginId = ?1 AND (c.contactId = ?2 OR m.contactId = ?2) GROUP BY m.conversationId;"

#define SQL_STMT_SELECT_MESSAGES "\
SELECT m.flags, m.aTimestamp, m.content FROM messages as m WHERE m.conversationId = ? ORDER BY m.id ASC;"

#define SQL_STMT_DELETE_CONTACT "DELETE FROM conversations WHERE pluginId = ? AND contactId = ?;"

#define SQL_STMT_DELETE_CONVERSATION "DELETE FROM conversations WHERE id = ?"

static CONST TEXT *SQL[] =
{
	SQL_STMT_INSERT_CONVERSATION,
	SQL_STMT_INSERT_MESSAGE,
	SQL_STMT_SELECT_LAST_MESSAGES,
	SQL_STMT_SELECT_LAST_CONVERSATION_MESSAGES,
	SQL_STMT_SELECT_LAST_MESSAGES_TIME,
	SQL_STMT_SELECT_LAST_CONVERSATION,
	SQL_STMT_SELECT_CONTACTS,
	SQL_STMT_SELECT_CONVERSATIONS,
	SQL_STMT_SELECT_MESSAGES,
	SQL_STMT_DELETE_CONTACT,
	SQL_STMT_DELETE_CONVERSATION
};

enum SQL_E
{
	SQL_INSERT_CONVERSATION = 0,
	SQL_INSERT_MESSAGE,
	SQL_SELECT_LAST_MESSAGES,
	SQL_SELECT_LAST_CONVERSATION_MESSAGES,
	SQL_SELECT_LAST_MESSAGES_TIME,
	SQL_SELECT_LAST_CONVERSATION,
	SQL_SELECT_CONTACTS,
	SQL_SELECT_CONVERSATIONS,
	SQL_SELECT_MESSAGES,
	SQL_DELETE_CONTACT,
	SQL_DELETE_CONVERSATION
};

#define SQL_STMT_NO (sizeof(SQL) / sizeof(*SQL))

#endif /* __HISTORYSQL_H__ */
