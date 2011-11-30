/*=========================================================================

  Project                 : XdmfGenerator
  Module                  : XdmfGenerator.cxx

  Authors:
     John Biddiscombe     Jerome Soumagne
     biddisco@cscs.ch     soumagne@cscs.ch

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  This work has received funding from the European Community's Seventh
  Framework Programme (FP7/2007-2013) under grant agreement 225967 “NextMuSE”

=========================================================================*/
#ifdef USE_VLD_MEMORY_LEAK_DETECTION
 #include "vld.h"
#endif
#include <libxml/tree.h>
#include "XdmfGenerator.h"
#include "XdmfArray.h"
//
#include <vtksys/RegularExpression.hxx>
//
#include "XdmfHDFDOM.h"
#include "XdmfGrid.h"
#include "XdmfTopology.h"
#include "XdmfGeometry.h"
#include "XdmfTime.h"
#include "XdmfAttribute.h"
#include "XdmfDataItem.h"
#include "XdmfDataDesc.h"

#ifdef USE_H5FD_DSM
#include "H5FDdsmManager.h"
#endif
#include "XdmfDump.h"

#include "FileSeriesFinder.h"

#include <cstdlib>
#include <stack>
//----------------------------------------------------------------------------
XdmfGenerator::XdmfGenerator()
{
  this->DsmManager       = NULL;
  this->GeneratedFile   = NULL;
  this->PrefixRegEx     = NULL;
  this->TimeRegEx       = NULL;
  this->ExtRegEx        = NULL;
  this->UseFullHDF5Path = XDMF_TRUE;
  // default file type: output.0000.h5
  this->SetPrefixRegEx("(.+).");
  this->SetTimeRegEx("([0-9]+)");
  this->SetExtRegEx("([.]h5)");

  // Set the generated DOM
  this->GeneratedRoot.SetDOM(&this->GeneratedDOM);
  this->GeneratedRoot.Build();
  this->GeneratedRoot.Insert(&this->GeneratedDomain);
  this->GeneratedRoot.Build();
}
//----------------------------------------------------------------------------
XdmfGenerator::~XdmfGenerator()
{
  if (this->GeneratedFile) free(this->GeneratedFile);
  this->GeneratedFile = NULL;
}
//----------------------------------------------------------------------------
XdmfDOM *XdmfGenerator::GetGeneratedDOM()
{
  return &this->GeneratedDOM;
}
//----------------------------------------------------------------------------
XdmfConstString XdmfGenerator::GetGeneratedFile()
{
  std::ostringstream  generatedFileStream;
  generatedFileStream << "<?xml version=\"1.0\" ?>" << endl
      << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>" << endl
      << this->GeneratedDOM.Serialize();
//  this->GeneratedFile = generatedFileStream.str();
  if (this->GeneratedFile) free(this->GeneratedFile);
  this->GeneratedFile = (XdmfString) malloc(generatedFileStream.str().size() + 1);
  strcpy(this->GeneratedFile, generatedFileStream.str().c_str());
  return (XdmfConstString)this->GeneratedFile;
}
//----------------------------------------------------------------------------
void XdmfGenerator::SetDsmManager(H5FDdsmManager *dsmManager)
{
    this->DsmManager = dsmManager;
}
//----------------------------------------------------------------------------
XdmfInt32 XdmfGenerator::GenerateTemporalCollection(XdmfConstString lXdmfFile,
    XdmfConstString anHdfFile, XdmfConstString fileNamePattern)
{
  vtkstd::vector<double> timeStepValues;
  XdmfGrid temporalGrid;
  XdmfFileSeriesFinder *fileFinder;

  if (fileNamePattern) {
    fileFinder = new XdmfFileSeriesFinder(fileNamePattern);
  } else {
    fileFinder = new XdmfFileSeriesFinder();
  }
  temporalGrid.SetGridType(XDMF_GRID_COLLECTION);
  temporalGrid.SetCollectionType(XDMF_GRID_COLLECTION_TEMPORAL);
  this->GeneratedDomain.Insert(&temporalGrid);
  // Build the temporal grid
  temporalGrid.Build();

  fileFinder->SetPrefixRegEx(this->PrefixRegEx);
  fileFinder->SetTimeRegEx(this->TimeRegEx);
  fileFinder->SetExtRegEx(this->ExtRegEx);
  fileFinder->Scan(anHdfFile);
  // TODO use time values of files eventually
  fileFinder->GetTimeValues(timeStepValues);

  // std::cerr << "Number of TimeSteps: " <<fileFinder->GetNumberOfTimeSteps() << std::endl;
  for (int i=0; i<fileFinder->GetNumberOfTimeSteps(); i++) {
    // std::cerr << "Generate file name for time step: " << timeStepValues[i] << std::endl;
    std::string currentHdfFile = fileFinder->GenerateFileName(i);
    // std::cerr << "File name generated: " << currentHdfFile << std::endl;
    this->Generate(lXdmfFile, currentHdfFile.c_str(), &temporalGrid, timeStepValues[i]);
  }
  delete fileFinder;
  return(XDMF_SUCCESS);
}
//----------------------------------------------------------------------------
XdmfInt32 XdmfGenerator::Generate(XdmfConstString lXdmfFile, XdmfConstString hdfFileName,
    XdmfGrid *temporalGrid, XdmfInt32 timeValue)
{
  XdmfXmlNode         domainNode;
  XdmfDOM            *lXdmfDOM = new XdmfDOM();
  XdmfHDFDOM         *hdfDOM = new XdmfHDFDOM();
  XdmfDump           *hdfFileDump = new XdmfDump();
  XdmfGrid           *spatialGrid = NULL;
  XdmfTime           *timeInfo = NULL;
  std::ostringstream  hdfFileDumpStream;

  if (!hdfFileName) {
    XdmfErrorMessage("HDF file name must be set before generating");
    return(XDMF_FAIL);
  }

  // Fill LXdmfDOM
  if (lXdmfDOM->Parse(lXdmfFile) != XDMF_SUCCESS) {
    XdmfErrorMessage("Unable to parse light XDMF xml file");
    return(XDMF_FAIL);
  }

  // Dump HDF file
  hdfFileDump->SetFileName(hdfFileName);
  if (this->DsmManager) {
    hdfFileDump->SetDsmManager(this->DsmManager);
  }
  hdfFileDump->DumpXML(hdfFileDumpStream);

//  std::cerr << hdfFileDumpStream.str().c_str() << std::endl;

  // Fill HDF DOM
  if (hdfDOM->Parse(hdfFileDumpStream.str().c_str()) != XDMF_SUCCESS) {
    XdmfErrorMessage("Unable to parse HDF xml file");
    return(XDMF_FAIL);
  }
//  hdfDOM->SetGlobalDebug(1);
//  std::cerr << (hdfDOM->Serialize()) << std::endl;

  //Find domain element
  domainNode = lXdmfDOM->FindElement("Domain");

  //
  // If the template has multiple grids, create a spatial grid 
  // to hold the multi-block structure
  //
  int numberOfGrids = lXdmfDOM->FindNumberOfElements("Grid", domainNode);
  int numberOfGrids2 = lXdmfDOM->FindNumberOfElements("Grid", domainNode);
  if (numberOfGrids>1) {
    // Use a spatial grid collection to store the subgrids
    spatialGrid = new XdmfGrid();
    spatialGrid->SetGridType(XDMF_GRID_COLLECTION);
    spatialGrid->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
    if (temporalGrid) {
      temporalGrid->Insert(spatialGrid);
    }
    else {
      this->GeneratedDomain.Insert(spatialGrid);
    }
    spatialGrid->SetDeleteOnGridDelete(1);
    spatialGrid->Build();
  }

  // Fill the GeneratedDOM
  XdmfGrid *grid = NULL;
  for (int currentGridIndex=0; currentGridIndex<numberOfGrids; currentGridIndex++) {
    bool abortgrid = false;
    grid = new XdmfGrid();
    XdmfTopology *topology;
    XdmfGeometry *geometry;
    XdmfXmlNode   gridNode       = lXdmfDOM->FindElement("Grid", currentGridIndex, domainNode);
    XdmfXmlNode   gridInfoDINode = NULL;
    XdmfXmlNode   topologyNode   = lXdmfDOM->FindElement("Topology", 0, gridNode);
    XdmfString    topologyTypeStr = NULL;
    XdmfXmlNode   topologyDINode  = NULL;
    XdmfXmlNode   geometryNode   = lXdmfDOM->FindElement("Geometry", 0, gridNode);
    XdmfString    geometryTypeStr = NULL;
    XdmfXmlNode   geometryDINode = NULL;
    // TODO Use timeNode
    //XdmfXmlNode   timeNode       = lXdmfDOM->FindElement("Time", 0, gridNode);

    // Set grid name
    XdmfString gridName = (XdmfString) lXdmfDOM->GetAttribute(gridNode, "Name");
    if (gridName) {
      grid->SetName(gridName);
      free(gridName);
    }
    else {
      gridInfoDINode = lXdmfDOM->FindElement("DataItem", 0, topologyNode);
      if (gridInfoDINode == NULL) {
        gridInfoDINode = lXdmfDOM->FindElement("DataItem", 0, geometryNode);
      }
      if (gridInfoDINode != NULL) {
        XdmfConstString gridInfoPath = lXdmfDOM->GetCData(gridInfoDINode);
        vtksys::RegularExpression gridName("^/([^/]*)/*");
        gridName.find(gridInfoPath);
        XdmfDebug("Grid name: " << gridName.match(1).c_str());
        grid->SetName(gridName.match(1).c_str());
      }
    }

    // Look for Topology
    topology = grid->GetTopology();
    topologyTypeStr = (XdmfString) lXdmfDOM->GetAttribute(topologyNode, "TopologyType");
    topology->SetTopologyTypeFromString(topologyTypeStr);
    topologyDINode = lXdmfDOM->FindElement("DataItem", 0, topologyNode);
    if(topologyDINode != NULL) {
      XdmfConstString topologyPath = lXdmfDOM->GetCData(topologyDINode);
      XdmfXmlNode hdfTopologyNode = this->FindConvertHDFPath(hdfDOM, topologyPath);
      if (!hdfTopologyNode) {
        XdmfDebug("Skipping node of path " << topologyPath);
        continue;
      }
      XdmfConstString topologyData = this->FindDataItemInfo(hdfDOM, hdfTopologyNode, hdfFileName, topologyPath, lXdmfDOM, topologyDINode);
      topology->SetNumberOfElements(this->FindNumberOfCells(hdfDOM, hdfTopologyNode, topologyTypeStr));
      topology->SetDataXml(topologyData);
      if (topologyData) delete []topologyData;
    }
    XdmfString topologyBaseOffset = (XdmfString) lXdmfDOM->GetAttribute(topologyNode, "BaseOffset");
    if (topologyBaseOffset) {
      topology->SetBaseOffset(atoi(topologyBaseOffset));
    }

    // Look for Geometry
    geometry = grid->GetGeometry();
    geometryTypeStr = (XdmfString) lXdmfDOM->GetAttribute(geometryNode, "GeometryType");
    geometry->SetGeometryTypeFromString(geometryTypeStr);
    if (geometryTypeStr) free(geometryTypeStr);
    geometryDINode = lXdmfDOM->FindElement("DataItem", 0, geometryNode);

    // we might need to read multiple items for x/y/z sizes (2-3 or more dimensions)
    std::vector<XdmfInt64> numcells;
    std::string geomXML = "<Geometry>";
    while (!abortgrid && geometryDINode != NULL) {
      XdmfConstString geometryPath = lXdmfDOM->GetCData(geometryDINode);
      XdmfXmlNode hdfGeometryNode = this->FindConvertHDFPath(hdfDOM, geometryPath);
      if (!hdfGeometryNode) {
        XdmfDebug("Skipping node of path " << geometryPath);
        // std::cerr << "Geometry Absent : Aborting Grid " << grid->GetName() << std::endl;
        abortgrid = true;
        continue;
      }
      XdmfConstString geometryData = this->FindDataItemInfo(hdfDOM, hdfGeometryNode, hdfFileName, geometryPath, lXdmfDOM, geometryDINode);
      if (geometryData) {
        geomXML += geometryData;
        delete []geometryData;
      }
      // Xdmf dimensions are in reverse order, so add each dimension to start of array, not end
      XdmfInt64 N = this->FindNumberOfCells(hdfDOM, hdfGeometryNode, topologyTypeStr);
      numcells.insert(numcells.begin(),N);
      //
      geometryDINode = geometryDINode->next;
    }
    if (abortgrid) {
      continue;
    }
    geomXML += "</Geometry>";
    geometry->SetDataXml(geomXML.c_str());
    //
    // The geometry may be {x,y,z} separate arrays, but this does not mean the topology is rank 3
    // for example particles, with x,y,z arrays are rank 1
    //
    if (topologyDINode == NULL) {
      switch (topology->GetTopologyType()) {
        case XDMF_2DSMESH      :  
        case XDMF_2DRECTMESH   :
        case XDMF_2DCORECTMESH :  
        case XDMF_3DSMESH      :  
        case XDMF_3DRECTMESH   :  
        case XDMF_3DCORECTMESH :  
          topology->GetShapeDesc()->SetShape((XdmfInt32)numcells.size(), &numcells[0]);
          break;
        default :
          topology->GetShapeDesc()->SetShape(1, &numcells[0]);
      }
    }
    if (topologyTypeStr) free(topologyTypeStr);

    if (spatialGrid) {
      spatialGrid->Insert(grid);
      grid->SetDeleteOnGridDelete(1);
    }
    else if (temporalGrid) {
      temporalGrid->Insert(grid);
      grid->SetDeleteOnGridDelete(1);
    }
    else {
      this->GeneratedDomain.Insert(grid);
    }

    //
    // Look for Attributes
    //
    int numberOfAttributes = lXdmfDOM->FindNumberOfElements("Attribute", gridNode);
    typedef struct attribNodeInfo {
      XdmfXmlNode attribNode;
      XdmfXmlNode attributeDINode;
      std::string path;
      std::string name;
      int AttributeType;
      //
      attribNodeInfo(XdmfXmlNode t, XdmfXmlNode h, std::string p, std::string n, int a) :
      attribNode(t), attributeDINode(h), path(p), name(n), AttributeType(a) {};
    } attribNodeInfo;

//    typedef std::pair<std::string, std::string> stringpair;
//    typedef std::pair<XdmfXmlNode, XdmfXmlNode> nodepair;
//    typedef std::pair<nodepair, stringpair> AttributeInfo;
    std::vector<attribNodeInfo> attributes;
    
    //
    // Loop over all attributes in the template and use wildcards to find 
    // those that are in the HDF, if not a wildcard, then use directly.
    //
    for (int currentIndex=0; currentIndex<numberOfAttributes; currentIndex++) {
      XdmfXmlNode attributeNode = lXdmfDOM->FindElement("Attribute", currentIndex, gridNode);
      if (attributeNode) {        
        XdmfXmlNode attributeDINode = lXdmfDOM->FindElement("DataItem", 0, attributeNode);
        // Set Attribute Name, use one from template if it exists
        XdmfConstString attributeName = lXdmfDOM->GetAttribute(attributeNode, "Name");
        XdmfConstString attribtype    = lXdmfDOM->GetAttribute(attributeNode, "AttributeType");
        XdmfConstString attributePath = lXdmfDOM->GetCData(attributeDINode);
        // for wildcards, find all the nodes in the HDF dump XML
        if (STRCASECMP(attributeName, "*")==0) {
          std::string wildcard_search = this->ConvertHDFPath(hdfDOM, attributePath);
          XdmfXmlNode node = hdfDOM->FindElementByPath(wildcard_search.c_str());
          while (node) {          
            XdmfConstString name = hdfDOM->GetAttribute(node, "Name");
            std::string path = attributePath;
            vtksys::SystemTools::ReplaceString(path,"*",name);
            int atype = this->FindAttributeType(hdfDOM, node, lXdmfDOM, attributeNode);
            // push a wildcard attribute (the node points to hdf xml, not template xml
            attributes.push_back(attribNodeInfo(node,attributeDINode,path,name,atype));
            node = node->next;
          }
        }
        else {
          // add a non wildcard attribute
          int atype = this->FindAttributeType(hdfDOM, NULL, lXdmfDOM, attributeNode);
          attributes.push_back(attribNodeInfo(attributeNode,attributeDINode,attributePath,attributeName,atype));
        }
        if (attributeName) free((void*)attributeName);
        if (attribtype) free((void*)attribtype);
      }
    }

    for (std::vector<attribNodeInfo>::iterator it=attributes.begin(); it!=attributes.end(); ++it) 
    {     
      XdmfXmlNode attributeNode   = (*it).attribNode;
      XdmfXmlNode attributeDINode = (*it).attributeDINode;
      std::string path = (*it).path;
      std::string name = (*it).name;

      //
      XdmfAttribute *attribute = new XdmfAttribute();
      attribute->SetName(name.c_str());

      // scalar/vector/tensor
      XdmfInt32 attributeDIType = this->FindDataItemType(lXdmfDOM, attributeDINode);
      // Check Data Item Type
      if (attributeDIType == XDMF_ITEM_FUNCTION) {

        std::string dataItemFunction = "";
        XdmfString functionName = (XdmfString) lXdmfDOM->GetAttribute(attributeDINode, "Function");
        if (!functionName) {
          XdmfErrorMessage("No function defined for attribute " << name.c_str());
        }

        XdmfXmlNode subDataItemNode = lXdmfDOM->FindElement("DataItem", 0, attributeDINode);
        XdmfInt32 numberOfSubDataItems = lXdmfDOM->FindNumberOfElements("DataItem", attributeDINode);
        XdmfDebug("Function DataItem has " << numberOfSubDataItems << " SubDataItems");
        std::string dataItemCData;

        XdmfConstString subDataItemHDFPath = lXdmfDOM->GetCData(subDataItemNode);
        XdmfXmlNode hdfSubDataItemNode = this->FindConvertHDFPath(hdfDOM, subDataItemHDFPath);
        XdmfXmlNode subDataItemHDFDataspaceNode = hdfDOM->FindElement("Dataspace", 0, hdfSubDataItemNode);
        if (!subDataItemHDFDataspaceNode) {
          XdmfErrorMessage("No Dataspace element found");
          return(XDMF_FAIL);
        }
        // Suppose we only have one dimensional arrays here
        XdmfString subDataItemDimSize = (XdmfString) hdfDOM->GetAttribute(hdfDOM->GetChild(0, hdfDOM->GetChild(0, subDataItemHDFDataspaceNode)), "DimSize");
        XdmfByte subDataItemDims[16];
        sprintf(subDataItemDims, "%d %s", numberOfSubDataItems, subDataItemDimSize);
        if (subDataItemDimSize) free(subDataItemDimSize);

        while (subDataItemNode != NULL) {
          // Get Item info
          subDataItemHDFPath = lXdmfDOM->GetCData(subDataItemNode);
          hdfSubDataItemNode = this->FindConvertHDFPath(hdfDOM, subDataItemHDFPath);
          XdmfConstString subDataItemData = this->FindDataItemInfo(hdfDOM, hdfSubDataItemNode, hdfFileName, subDataItemHDFPath, lXdmfDOM, subDataItemNode);
          if (subDataItemData) {
            dataItemCData += subDataItemData;
            delete []subDataItemData;
          }
          subDataItemNode = subDataItemNode->next;
        }

        std::string dataItem =
            std::string("<DataItem ") +
            "Dimensions=\"" + std::string(subDataItemDims) + "\" " +
            "Function=\"" + std::string(functionName) + "\" " +
            "ItemType=\"Function\">" +
            dataItemCData +
            "</DataItem>";

        free(functionName);

        attribute->SetDataXml(dataItem.c_str());
      } 
      else {
        // for wildcard attributes we don't need to call this as the node is already found  
        attributeNode = this->FindConvertHDFPath(hdfDOM, path.c_str());

        if (!attributeNode) {
          // The node does not exist in the HDF DOM so do not generate it
          XdmfDebug("Skipping node of path " << path.c_str());
          continue;
        }

        // Get Attribute info
        XdmfConstString attributeData = this->FindDataItemInfo(hdfDOM, attributeNode, hdfFileName, path.c_str(), lXdmfDOM, attributeDINode);
        attribute->SetDataXml(attributeData);
        if (attributeData) delete []attributeData;
      }

      // Set node center by default at the moment
      attribute->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
      //
      // when using wildcards, we must use the attribute type scalar/vector/tensor from
      // the wildcard node as we can't guess it well from the dataset
      //
      attribute->SetAttributeType((*it).AttributeType);

      grid->Insert(attribute);
      attribute->SetDeleteOnGridDelete(1);
    }
    
    grid->Build();

    //
    // After Grid build has been called, our geometry tag is wrong since we added a composite
    // {x,y,z} XML tag instead of a simple one, so delete one level of xml nodes from 
    // each valid Geometry tag (use stack to handle multiple blocks etc)
    //
    XdmfXmlNode domainNode2 = this->GeneratedDOM.FindElement("Domain");
    XdmfXmlNode   gridNode2 = this->GeneratedDOM.FindElement("Grid", 0, domainNode2);
    std::stack<XdmfXmlNode> nodestack;
    nodestack.push(gridNode2);
    XdmfXmlNode node;
    while (!nodestack.empty()) {
      node = nodestack.top();
      nodestack.pop();
      // Does this grid have child grids, if so push onto stack
      XdmfXmlNode child = this->GeneratedDOM.FindElement("Grid", 0, node);
      if (child) nodestack.push(child);
      // Does this grid have sibling grids, if so push onto stack
      XdmfXmlNode sibling = this->GeneratedDOM.FindNextElement("Grid", node);
      if (sibling) nodestack.push(sibling);
      // Does the node have a Geometry node?
      XdmfXmlNode geometryNode2 = this->GeneratedDOM.FindElement("Geometry", 0, node);
      if (geometryNode2) {
        // replace the Geometry node with its (real) child Geometry node
        XdmfXmlNode tmp = geometryNode2->children;
        if (tmp && !xmlStrcmp(tmp->name, BAD_CAST("Geometry"))) {
          geometryNode2->children = geometryNode2->children->children;
          tmp->children = NULL;
          xmlFreeNode(tmp);
        }
      }
    }

    // Normally container deletes the grid, if no container, we must do it
    if (!spatialGrid && !temporalGrid) {
      delete grid;
    }

  }
  if (temporalGrid) {
    // Look for Time
    // TODO Add Time Node / enhancements??
    timeInfo = new XdmfTime();
    timeInfo->SetTimeType(XDMF_TIME_SINGLE);
    timeInfo->SetValue(timeValue);
    if (spatialGrid) spatialGrid->Insert(timeInfo);
    else if (grid) grid->Insert(timeInfo);
    timeInfo->Build();
    delete timeInfo;
  }
  delete hdfDOM;
  delete lXdmfDOM;
  delete hdfFileDump;

  return(XDMF_SUCCESS);
}
//----------------------------------------------------------------------------
std::string XdmfGenerator::ConvertHDFPath(XdmfHDFDOM *hdfDOM, XdmfConstString path)
{
  std::string newPath = "/HDF5-File/RootGroup/";
  std::string currentBlockName = "";

  // skip leading "/"
  int cursor = 1;

  while (path[cursor] != '\0') {
    if (path[cursor] == '/') {
      newPath += "Group[@Name=\"" + currentBlockName + "\"]/";
      currentBlockName.clear();
      currentBlockName = "";
    } else {
      currentBlockName += path[cursor];
    }
    cursor++;
  }

  if (currentBlockName=="*") {
    newPath += "Dataset[*]";
  } 
  else {
    newPath += "Dataset[@Name=\"" + currentBlockName + "\"]";
  }
  return newPath;
}
//----------------------------------------------------------------------------
XdmfXmlNode XdmfGenerator::FindConvertHDFPath(XdmfHDFDOM *hdfDOM, XdmfConstString path)
{
  std::string newPath = ConvertHDFPath(hdfDOM, path);
  XdmfXmlNode node = hdfDOM->FindElementByPath(newPath.c_str());
  return node;
}
//----------------------------------------------------------------------------
XdmfInt32 XdmfGenerator::FindNumberOfCells(XdmfHDFDOM *hdfDOM,
    XdmfXmlNode hdfTopologyNode, XdmfConstString topologyTypeStr)
{
  XdmfInt32   numberOfCells = 0;
  std::string topologyType = topologyTypeStr;
  XdmfXmlNode hdfDataspaceNode;
  XdmfInt32   dimSize;
  XdmfString  dimSizeStr = NULL;

  for (int i=0; i<(int)topologyType.length(); i++) {
    topologyType[i] = toupper(topologyType[i]);
  }

  hdfDataspaceNode = hdfDOM->FindElement("Dataspace", 0, hdfTopologyNode);
  dimSizeStr = (XdmfString) hdfDOM->GetAttribute(hdfDOM->GetChild(0,
      hdfDOM->GetChild(0, hdfDataspaceNode)), "DimSize");
  dimSize = atoi(dimSizeStr);
  if (dimSizeStr) free(dimSizeStr);

  // TODO Do other topology types
  if (topologyType == "MIXED") {
    numberOfCells = dimSize - 1;
  }
  else if (topologyType == "POLYVERTEX") {
    numberOfCells = dimSize;
  }
  else if (topologyType == "3DRECTMESH") {
    numberOfCells = dimSize;
  }
  else { 
    numberOfCells = dimSize;
  }

  return numberOfCells;
}
//----------------------------------------------------------------------------
XdmfConstString XdmfGenerator::FindDataItemInfo(XdmfHDFDOM *hdfDOM, XdmfXmlNode hdfDatasetNode,
    XdmfConstString hdfFileName, XdmfConstString dataPath, XdmfDOM *lXdmfDOM, XdmfXmlNode templateNode)
{
  XdmfXmlNode hdfDataspaceNode = NULL, hdfDatatypeNode = NULL;
  XdmfString nDimsStr, dataPrecisionStr, dataItemStr;
  XdmfInt32 nDims;
  std::string dimSize, hdfDataType, dataType, dataPrecision, dataItem;

  hdfDataspaceNode = hdfDOM->FindElement("Dataspace", 0, hdfDatasetNode);
  if (!hdfDataspaceNode) {
    XdmfErrorMessage("No Dataspace element found");
    return NULL;
  }
  nDimsStr = (XdmfString) hdfDOM->GetAttribute(hdfDOM->GetChild(0, hdfDataspaceNode), "Ndims");
  nDims = atoi(nDimsStr);
  if (nDimsStr) free(nDimsStr);

  // The attribute type (Scalar/Vector/Tensor/None) should be detected from the 
  // size of the first dimension - so 3x64x64x64 is a cube(64) of vectors
  // and 1x32x32 is a plane of scalars
  for (int i=0; i<nDims; i++) {
    XdmfString dimSizeStr = (XdmfString) hdfDOM->GetAttribute(
        hdfDOM->GetChild(i, hdfDOM->GetChild(0, hdfDataspaceNode)), "DimSize");
    dimSize += dimSizeStr;
    if (i != (nDims-1)) dimSize += " ";
    if (dimSizeStr) free(dimSizeStr);
  }

  hdfDatatypeNode = hdfDOM->FindElement("DataType", 0, hdfDatasetNode);
  if (!hdfDatatypeNode) {
    XdmfErrorMessage("No DataType element found");
    return NULL;
  }
  hdfDataType = hdfDOM->GetElementName(hdfDOM->GetChild(0, hdfDOM->GetChild(0, hdfDatatypeNode)));

  // Float | Int | UInt | Char | UChar
  if (hdfDataType == "IntegerType") {
    dataType = "Int";
  }
  else if (hdfDataType == "FloatType") {
    dataType = "Float";
  }

  dataPrecisionStr = (XdmfString) hdfDOM->GetAttribute(hdfDOM->GetChild(0,
      hdfDOM->GetChild(0, hdfDatatypeNode)), "Size");
  dataPrecision = dataPrecisionStr;
  if (dataPrecisionStr) free(dataPrecisionStr);

  std::string diName = "";
  if (templateNode) {
    XdmfConstString nodeName = lXdmfDOM->GetAttribute(templateNode, "Name");
    if (nodeName) {
      diName = "Name=\"" + std::string(nodeName) + "\" ";
    }
  }

  // 1) Never use windows style slashes in hdf paths
  // 2) Only use the relative file name, drop the path 
  // otherwise as you can't copy hdf5 + xml files between locations
  std::string unixname = hdfFileName;
  std::replace(unixname.begin(), unixname.end(), '\\', '/');
  size_t found = unixname.find_last_of("/\\");
  if (!this->UseFullHDF5Path) {
    unixname = unixname.substr(found+1);
  } else {
    if (!this->DsmManager) {
      unixname = "File:" + unixname;
    }
  }

  // TODO Instead of using a string, may replace this by using XdmfDataItem
  dataItem =
      std::string("<DataItem ") +
      diName + 
      "Dimensions=\"" + dimSize + "\" " +
      "NumberType=\"" + dataType + "\" " +
      "Precision=\"" + dataPrecision + "\" " +
      "Format=\"HDF\">" +
      unixname + ":" + std::string(dataPath) +
      "</DataItem>";
  dataItemStr = new char[dataItem.length()+1];
  strcpy(dataItemStr, dataItem.c_str());

  return (XdmfConstString)dataItemStr;
}
//----------------------------------------------------------------------------
XdmfInt32 XdmfGenerator::FindAttributeType(XdmfHDFDOM *hdfDOM, XdmfXmlNode hdfDatasetNode, XdmfDOM *lXdmfDOM, XdmfXmlNode templateNode)
{
  XdmfXmlNode hdfDataspaceNode;
  XdmfInt32 nDims;
  XdmfString nDimsStr, attrType;

  // if the template has defined the attribute type, use it
  attrType = (XdmfString) lXdmfDOM->GetAttribute(templateNode, "AttributeType");
  if (attrType) {
    XdmfInt32 aType = XDMF_ATTRIBUTE_TYPE_NONE;
    if      (!strcmp(attrType,"Scalar")) aType = XDMF_ATTRIBUTE_TYPE_SCALAR;
    else if (!strcmp(attrType,"Vector")) aType = XDMF_ATTRIBUTE_TYPE_VECTOR;
    else if (!strcmp(attrType,"Tensor")) aType = XDMF_ATTRIBUTE_TYPE_TENSOR;
    if (attrType) free(attrType);
    return aType;
  }

  if (hdfDatasetNode) {
    // Otherwise, we can't be sure, but if only one dimentsion, it must be scalar
    hdfDataspaceNode = hdfDOM->FindElement("Dataspace", 0, hdfDatasetNode);
    nDimsStr = (XdmfString) hdfDOM->GetAttribute(hdfDOM->GetChild(0, hdfDataspaceNode), "Ndims");
    nDims = atoi(nDimsStr);
    if (nDimsStr) free(nDimsStr);

    // This is unreliable, if ndims==1 then scalar is correct, otherwise we are guessing.
    switch (nDims) {
    case 1:
      return XDMF_ATTRIBUTE_TYPE_SCALAR;
    case 2:
      return XDMF_ATTRIBUTE_TYPE_VECTOR;
    default:
      return XDMF_ATTRIBUTE_TYPE_NONE;
    }
  }
  return XDMF_ATTRIBUTE_TYPE_NONE;
}
//----------------------------------------------------------------------------
XdmfInt32 XdmfGenerator::FindDataItemType(XdmfDOM *lXdmfDOM, XdmfXmlNode dataItemNode)
{
  XdmfConstString dataItemType;

  // if the template has defined the attribute type, use it
  dataItemType = lXdmfDOM->GetAttribute(dataItemNode, "ItemType");
  if (dataItemType) {
    XdmfInt32 aType = XDMF_ITEM_UNIFORM;

    if(XDMF_WORD_CMP(dataItemType, "Uniform")){
      aType = XDMF_ITEM_UNIFORM;
    }
    else if(XDMF_WORD_CMP(dataItemType, "Collection")){
      aType = XDMF_ITEM_COLLECTION;
    }
    else if(XDMF_WORD_CMP(dataItemType, "Tree")){
      aType = XDMF_ITEM_TREE;
    }
    else if(XDMF_WORD_CMP(dataItemType, "HyperSlab")){
      aType = XDMF_ITEM_HYPERSLAB;
    }
    else if(XDMF_WORD_CMP(dataItemType, "Coordinates")){
      aType = XDMF_ITEM_COORDINATES;
    }
    else if(XDMF_WORD_CMP(dataItemType, "Function")){
      aType = XDMF_ITEM_FUNCTION;
    }

    free((XdmfString)dataItemType);
    return aType;
  }
  return XDMF_ITEM_UNIFORM;
}
//----------------------------------------------------------------------------

