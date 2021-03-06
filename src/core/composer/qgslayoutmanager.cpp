/***************************************************************************
    qgslayoutmanager.cpp
    --------------------
    Date                 : January 2017
    Copyright            : (C) 2017 Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslayoutmanager.h"
#include "qgslayout.h"
#include "qgsproject.h"
#include "qgslogger.h"
#include "qgslayoutundostack.h"

QgsLayoutManager::QgsLayoutManager( QgsProject *project )
  : QObject( project )
  , mProject( project )
{

}

QgsLayoutManager::~QgsLayoutManager()
{
  clear();
}

bool QgsLayoutManager::addComposition( QgsComposition *composition )
{
  if ( !composition )
    return false;

  // check for duplicate name
  Q_FOREACH ( QgsComposition *c, mCompositions )
  {
    if ( c->name() == composition->name() )
      return false;
  }

  connect( composition, &QgsComposition::nameChanged, this, [this, composition]( const QString & newName )
  {
    emit compositionRenamed( composition, newName );
  } );
  emit compositionAboutToBeAdded( composition->name() );
  mCompositions << composition;
  emit compositionAdded( composition->name() );
  mProject->setDirty( true );
  return true;
}

bool QgsLayoutManager::addLayout( QgsLayout *layout )
{
  if ( !layout )
    return false;

  // check for duplicate name
  for ( QgsLayout *l : qgis::as_const( mLayouts ) )
  {
    if ( l->name() == layout->name() )
      return false;
  }

  connect( layout, &QgsLayout::nameChanged, this, [this, layout]( const QString & newName )
  {
    emit layoutRenamed( layout, newName );
  } );
  emit layoutAboutToBeAdded( layout->name() );
  mLayouts << layout;
  emit layoutAdded( layout->name() );
  mProject->setDirty( true );
  return true;
}

bool QgsLayoutManager::removeComposition( QgsComposition *composition )
{
  if ( !composition )
    return false;

  if ( !mCompositions.contains( composition ) )
    return false;

  QString name = composition->name();
  emit compositionAboutToBeRemoved( name );
  mCompositions.removeAll( composition );
  delete composition;
  emit compositionRemoved( name );
  mProject->setDirty( true );
  return true;
}

bool QgsLayoutManager::removeLayout( QgsLayout *layout )
{
  if ( !layout )
    return false;

  if ( !mLayouts.contains( layout ) )
    return false;

  QString name = layout->name();
  emit layoutAboutToBeRemoved( name );
  mLayouts.removeAll( layout );
  delete layout;
  emit layoutRemoved( name );
  mProject->setDirty( true );
  return true;
}

void QgsLayoutManager::clear()
{
  Q_FOREACH ( QgsComposition *c, mCompositions )
  {
    removeComposition( c );
  }
  const QList< QgsLayout * > layouts = mLayouts;
  for ( QgsLayout *l : layouts )
  {
    removeLayout( l );
  }
}

QList<QgsComposition *> QgsLayoutManager::compositions() const
{
  return mCompositions;
}

QList<QgsLayout *> QgsLayoutManager::layouts() const
{
  return mLayouts;
}

QgsComposition *QgsLayoutManager::compositionByName( const QString &name ) const
{
  Q_FOREACH ( QgsComposition *c, mCompositions )
  {
    if ( c->name() == name )
      return c;
  }
  return nullptr;
}

QgsLayout *QgsLayoutManager::layoutByName( const QString &name ) const
{
  for ( QgsLayout *l : mLayouts )
  {
    if ( l->name() == name )
      return l;
  }
  return nullptr;
}

bool QgsLayoutManager::readXml( const QDomElement &element, const QDomDocument &doc )
{
  clear();

  QDomElement layoutsElem = element;
  if ( element.tagName() != QStringLiteral( "Layouts" ) )
  {
    layoutsElem = element.firstChildElement( QStringLiteral( "Layouts" ) );
  }
  if ( layoutsElem.isNull() )
  {
    // handle legacy projects
    layoutsElem = doc.documentElement();
  }

  //restore each composer
  bool result = true;
  QDomNodeList composerNodes = element.elementsByTagName( QStringLiteral( "Composer" ) );
  for ( int i = 0; i < composerNodes.size(); ++i )
  {
    QString legacyTitle = composerNodes.at( i ).toElement().attribute( QStringLiteral( "title" ) );
    QgsComposition *c = createCompositionFromXml( composerNodes.at( i ).toElement(), doc );
    if ( !c )
    {
      result = false;
      continue;
    }
    if ( c->name().isEmpty() )
      c->setName( legacyTitle );
    result = result && addComposition( c );
  }

  QgsReadWriteContext context;
  context.setPathResolver( mProject->pathResolver() );

  // restore layouts
  const QDomNodeList layoutNodes = element.elementsByTagName( QStringLiteral( "Layout" ) );
  for ( int i = 0; i < layoutNodes.size(); ++i )
  {
    std::unique_ptr< QgsLayout > l = qgis::make_unique< QgsLayout >( mProject );
    l->undoStack()->blockCommands( true );
    if ( !l->readXml( layoutNodes.at( i ).toElement(), doc, context ) )
    {
      result = false;
      continue;
    }
    l->undoStack()->blockCommands( false );
    if ( addLayout( l.get() ) )
    {
      ( void )l.release(); // ownership was transferred successfully
    }
    else
    {
      result = false;
    }
  }
  return result;
}

QDomElement QgsLayoutManager::writeXml( QDomDocument &doc ) const
{
  QDomElement layoutsElem = doc.createElement( QStringLiteral( "Layouts" ) );
  Q_FOREACH ( QgsComposition *c, mCompositions )
  {
    QDomElement composerElem = doc.createElement( QStringLiteral( "Composer" ) );

    layoutsElem.appendChild( composerElem );

    c->writeXml( composerElem, doc );
    c->atlasComposition().writeXml( composerElem, doc );
  }
  QgsReadWriteContext context;
  context.setPathResolver( mProject->pathResolver() );
  for ( QgsLayout *l : mLayouts )
  {
    QDomElement layoutElem = l->writeXml( doc, context );
    layoutsElem.appendChild( layoutElem );
  }
  return layoutsElem;
}

bool QgsLayoutManager::saveAsTemplate( const QString &name, QDomDocument &doc ) const
{
  QgsComposition *c = compositionByName( name );
  if ( !c )
    return false;

  QDomElement composerElem = doc.createElement( QStringLiteral( "Composer" ) );
  doc.appendChild( composerElem );
  c->writeXml( composerElem, doc );
  c->atlasComposition().writeXml( composerElem, doc );
  return true;
}

QgsComposition *QgsLayoutManager::duplicateComposition( const QString &name, const QString &newName )
{
  QDomDocument currentDoc;
  if ( !saveAsTemplate( name, currentDoc ) )
    return nullptr;

  QDomElement compositionElem = currentDoc.documentElement().firstChildElement( QStringLiteral( "Composition" ) );
  if ( compositionElem.isNull() )
  {
    QgsDebugMsg( "selected composer could not be stored as temporary template" );
    return nullptr;
  }

  QgsComposition *newComposition( new QgsComposition( mProject ) );
  if ( !newComposition->loadFromTemplate( currentDoc, nullptr, false, true ) )
  {
    delete newComposition;
    return nullptr;
  }

  newComposition->setName( newName );
  if ( !addComposition( newComposition ) )
  {
    delete newComposition;
    return nullptr;
  }
  else
  {
    return newComposition;
  }
}

QgsLayout *QgsLayoutManager::duplicateLayout( const QgsLayout *layout, const QString &newName )
{
  QDomDocument currentDoc;

  QgsReadWriteContext context;
  QDomElement elem = layout->writeXml( currentDoc, context );
  currentDoc.appendChild( elem );

  std::unique_ptr< QgsLayout > newLayout = qgis::make_unique< QgsLayout >( mProject );
  bool ok = false;
  newLayout->loadFromTemplate( currentDoc, context, true, &ok );
  if ( !ok )
  {
    return nullptr;
  }

  newLayout->setName( newName );
  QgsLayout *l = newLayout.get();
  if ( !addLayout( l ) )
  {
    return nullptr;
  }
  else
  {
    ( void )newLayout.release(); //ownership was transferred successfully
    return l;
  }
}

QString QgsLayoutManager::generateUniqueComposerTitle() const
{
  QStringList names;
  Q_FOREACH ( QgsComposition *c, mCompositions )
  {
    names << c->name();
  }
  QString name;
  int id = 1;
  while ( name.isEmpty() || names.contains( name ) )
  {
    name = tr( "Composer %1" ).arg( id );
    id++;
  }
  return name;
}

QString QgsLayoutManager::generateUniqueTitle() const
{
  QStringList names;
  for ( QgsLayout *l : mLayouts )
  {
    names << l->name();
  }
  QString name;
  int id = 1;
  while ( name.isEmpty() || names.contains( name ) )
  {
    name = tr( "Layout %1" ).arg( id );
    id++;
  }
  return name;
}

QgsComposition *QgsLayoutManager::createCompositionFromXml( const QDomElement &element, const QDomDocument &doc ) const
{
  QDomNodeList compositionNodeList = element.elementsByTagName( QStringLiteral( "Composition" ) );

  if ( compositionNodeList.size() > 0 )
  {
    std::unique_ptr< QgsComposition > c( new QgsComposition( mProject ) );
    QDomElement compositionElem = compositionNodeList.at( 0 ).toElement();
    if ( !c->readXml( compositionElem, doc ) )
    {
      return nullptr;
    }

    // read atlas parameters - must be done before adding items
    QDomElement atlasElem = element.firstChildElement( QStringLiteral( "Atlas" ) );
    c->atlasComposition().readXml( atlasElem, doc );

    //read and restore all the items
    c->addItemsFromXml( element, doc );

    //make sure z values are consistent
    c->refreshZList();
    return c.release();
  }
  else
  {
    return nullptr;
  }
}
