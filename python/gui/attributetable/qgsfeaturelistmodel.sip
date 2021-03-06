/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/attributetable/qgsfeaturelistmodel.h                         *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/





class QgsFeatureListModel : QAbstractProxyModel, QgsFeatureModel
{

%TypeHeaderCode
#include "qgsfeaturelistmodel.h"
%End
  public:
    struct FeatureInfo
    {
      public:

        FeatureInfo();
%Docstring
Constructor for FeatureInfo.
%End

        bool isNew;
        bool isEdited;
    };

    enum Role
    {
      FeatureInfoRole,
      FeatureRole
    };

  public:

    explicit QgsFeatureListModel( QgsAttributeTableFilterModel *sourceModel, QObject *parent /TransferThis/ = 0 );
%Docstring
Constructor for QgsFeatureListModel
%End

    virtual void setSourceModel( QgsAttributeTableFilterModel *sourceModel );
    QgsVectorLayerCache *layerCache();
    virtual QVariant data( const QModelIndex &index, int role ) const;

    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;


    void setInjectNull( bool injectNull );
%Docstring
 If true is specified, a NULL value will be injected

:param injectNull: state of null value injection

.. versionadded:: 2.9
%End

    bool injectNull();
%Docstring
 Returns the current state of null value injection

:return: If a NULL value is added

.. versionadded:: 2.9
%End

    QgsAttributeTableModel *masterModel();

    bool setDisplayExpression( const QString &expression );
%Docstring

\param  expression   A QgsExpression compatible string.

:return: true if the expression could be set, false if there was a parse error.
If it fails, the old expression will still be applied. Call parserErrorString()
for a meaningful error message.
%End

    QString parserErrorString();
%Docstring
 Returns a detailed message about errors while parsing a :py:class:`QgsExpression`.

:return: A message containing information about the parser error.
%End

    QString displayExpression() const;
    bool featureByIndex( const QModelIndex &index, QgsFeature &feat );
    QgsFeatureId idxToFid( const QModelIndex &index ) const;
    QModelIndex fidToIdx( const QgsFeatureId fid ) const;

    virtual QModelIndex mapToSource( const QModelIndex &proxyIndex ) const;

    virtual QModelIndex mapFromSource( const QModelIndex &sourceIndex ) const;


    virtual QModelIndex mapToMaster( const QModelIndex &proxyIndex ) const;
    virtual QModelIndex mapFromMaster( const QModelIndex &sourceIndex ) const;

    virtual QItemSelection mapSelectionFromMaster( const QItemSelection &selection ) const;
    virtual QItemSelection mapSelectionToMaster( const QItemSelection &selection ) const;

    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;

    virtual QModelIndex parent( const QModelIndex &child ) const;

    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;

    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;


    virtual QModelIndex fidToIndex( QgsFeatureId fid );

    QModelIndexList fidToIndexList( QgsFeatureId fid );

  public slots:
    void onBeginRemoveRows( const QModelIndex &parent, int first, int last );
    void onEndRemoveRows( const QModelIndex &parent, int first, int last );
    void onBeginInsertRows( const QModelIndex &parent, int first, int last );
    void onEndInsertRows( const QModelIndex &parent, int first, int last );

};


/************************************************************************
 * This file has been generated automatically from                      *
 *                                                                      *
 * src/gui/attributetable/qgsfeaturelistmodel.h                         *
 *                                                                      *
 * Do not edit manually ! Edit header and run scripts/sipify.pl again   *
 ************************************************************************/
