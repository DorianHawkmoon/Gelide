/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gelide
 * Copyright (C) 2008 - 2014 Juan Ángel Moreno Fernández
 *
 * gelide is free software.
 *
 * You can redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * gelide is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gelide.  If not, see <http://www.gnu.org/licenses/>
 */

#include "db_manager.hpp"


namespace gelide{

Tag* DbManager::tagGet(const long long int id)
{
	SqliteStatement* stm;
	Tag* element;

	assert(m_db.isOpen());

	// Creamos el comando sql para obtener el género
	stm = m_db.createStatement(
			"SELECT * \n"
			"FROM Tags\n"
			"WHERE Id = :id"
	);
	if (!stm)
	{
		return NULL;
	}
	stm->bind(1, id);

	// Obtenemos el el elemento si existe en la bd
	if (stm->step() == SqliteStatement::STATEMENT_ROW)
	{
		element = new Tag(stm->getColumnInt64(0), stm->getColumnText(1));
	}
	else
	{
		element = NULL;
	}
	stm->finalize();
	delete stm;

	return element;
}

bool DbManager::tagAdd(Tag* tag)
{
	SqliteStatement* stm;
	int ret;

	assert(m_db.isOpen());
	assert(tag);
	assert(!tag->name.empty());

	// Preparamos el comando sql para la inserción y asociamos los valores
	stm = m_db.createStatement(
			"INSERT INTO Tags (Name)\n"
			"VALUES (:name)"
	);
	if (!stm)
	{
		return false;
	}
	stm->bind(1, tag->name);

	// Ejecutamos y limpiamos el comando
	ret = stm->step();
	stm->finalize();
	delete stm;

	// Si se ha añadido correctamente, obtenemos el identificador
	if (ret == SqliteStatement::STATEMENT_DONE)
	{
		tag->id = m_db.getLastInsertId();
		return true;
	}

	return false;
}

bool DbManager::tagUpdate(Tag* tag)
{
	SqliteStatement* stm;
	int ret;

	assert(m_db.isOpen());
	assert(tag);
	assert(tag->id);
	assert(!tag->name.empty());

	// En las actualizaciones, no se modifica el id
	stm = m_db.createStatement(
			"UPDATE Tags\n"
			"SET Name = :name\n"
			"WHERE Id = :id"
	);
	if (!stm)
	{
		return false;
	}
	stm->bind(1, tag->name);
	stm->bind(2, tag->id);

	ret = stm->step();
	stm->finalize();
	delete stm;

	if (ret == SqliteStatement::STATEMENT_DONE)
	{
		return true;
	}

	return false;
}

bool DbManager::tagDelete(const long long int id)
{
	SqliteStatement* stm;

	assert(m_db.isOpen());
	assert(id);

	// Realizamos la operación en una transacción
	m_db.transactionBegin();
		// Preparamos el comando para eliminar el género
		stm = m_db.createStatement(
				"DELETE FROM Tags\n"
				"WHERE Id = :id"
		);
		if (!stm)
		{
			m_db.transactionRollBack();
			return false;
		}
		stm->bind(1, id);

		if (stm->step() != SqliteStatement::STATEMENT_DONE)
		{
			stm->finalize();
			delete stm;
			m_db.transactionRollBack();
			return false;
		}
		stm->finalize();

		// Preparamos el comando para eliminar los juegos etiquetados
		if (!stm->prepare(
				"DELETE FROM TagEntries\n"
				"WHERE TagId = :id"
			)
		)
		{
			stm->finalize();
			delete stm;
			m_db.transactionRollBack();
			return false;
		}
		stm->bind(1, id);

		// Ejecutamos
		if (stm->step() != SqliteStatement::STATEMENT_DONE)
		{
			stm->finalize();
			delete stm;
			m_db.transactionRollBack();
			return false;
		}
		stm->finalize();

		delete stm;
	m_db.transactionCommit();

	// Desactivamos el filtro activo si coincide con el que eliminamos
	if ((m_filter_type == DBFILTER_TAG) && (m_filter_value == id))
	{
		m_filter_type = DBFILTER_NONE;
		m_filter_value = 0;
	}

	return true;
}

bool DbManager::tagGetAll(std::vector<Tag* >& list)
{
	SqliteStatement* stm;
	std::vector<Tag* >::iterator iter;
	int ret;
	Tag* element;

	assert(m_db.isOpen());

	// Creamos el comando sql para obtener los elementos
	stm = m_db.createStatement(
			"SELECT * \n"
			"FROM Tags\n"
			"ORDER BY Name"
	);
	if (!stm)
	{
		return false;
	}

	for (iter = list.begin(); iter != list.end(); ++iter)
	{
		delete (*iter);
	}
	list.clear();

	// Obtenemos los elementos de la base de datos
	while ((ret = stm->step()) == SqliteStatement::STATEMENT_ROW)
	{
		element = new Tag(stm->getColumnInt64(0), stm->getColumnText(1));
		list.push_back(element);
	}
	stm->finalize();
	delete stm;

	return true;
}

} // namespace gelide