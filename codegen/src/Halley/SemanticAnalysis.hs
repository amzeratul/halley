{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}
module Halley.SemanticAnalysis
( semanticAnalysis
, CodeGenData(components, systems)
, ComponentData(componentName, componentIndex, members, functions)
, SystemData(systemName, families)
, VariableData(variableName, variableType)
, VariableTypeData(typeName, const)
, FunctionData(functionName, returnType, arguments)
, FamilyData(familyName, familyComponents)
) where

import Halley.AST
import Control.Monad
import Data.Maybe
import Data.List
import Data.Either

data CodeGenData = CodeGenData { components :: [ComponentData]
                               , systems :: [SystemData]
                               } deriving (Show)

data ComponentData = ComponentData { componentName :: String
                                   , componentIndex :: Int
                                   , members :: [VariableData]
                                   , functions :: [FunctionData]
                                   } deriving (Show)

data SystemData = SystemData { systemName :: String
                             , families :: [FamilyData]
                             } deriving (Show)

data FamilyData = FamilyData { familyName :: String
                             , familyComponents :: [VariableTypeData]
                             } deriving (Show)

data VariableData = VariableData { variableName :: String
                                 , variableType :: VariableTypeData
                                 } deriving (Show)

data FunctionData = FunctionData { functionName :: String
                                 , returnType :: VariableTypeData
                                 , arguments :: [VariableData]
                                 } deriving (Show)

data VariableTypeData = VariableTypeData { typeName :: String
                                         , const :: Bool
                                         } deriving (Show, Eq)
---------------------


semanticAnalysis :: [GenDefinition] -> Either String CodeGenData
semanticAnalysis gens = case doValidate of
    Just error -> Left error
    Nothing -> Right loadedData
    where
        cs = map (loadComponent) [(name, entries) | ComponentDefinition name entries <- gens]
        sys = map (loadSystem) [(name, entries) | SystemDefinition name entries <- gens]
        loadedData = CodeGenData { components = indexComponents $ rights cs, systems = rights sys }
        doValidate = case lefts cs ++ lefts sys of
            [] -> validate loadedData
            es -> Just (intercalate "\n\n" es)

indexComponents :: [ComponentData] -> [ComponentData]
indexComponents cs = [c { componentIndex = i } | (c, i) <- zip cs [0..]]

loadComponent :: (String, [GenEntryDefinition]) -> Either String ComponentData
loadComponent (name, entries) = case getError of
    Just e -> Left ("Error with component " ++ name ++ ": " ++ e)
    Nothing -> Right ComponentData { componentName = name, componentIndex = -1, members = rights getMembers, functions = rights getFunctions }
    where
        memberLists = [vars | MemberList vars <- entries]
        functionLists = [funcs | FunctionList funcs <- entries]
        getMembers = map (loadVariable) $ tryHead memberLists
        getFunctions = [] -- TODO
        errorList = lefts getMembers ++ lefts getFunctions
        getError
            | length memberLists > 1 = Just "Multiple member lists declared"
            | length functionLists > 1 = Just "Multiple function lists declared"
            | not $ null errorList = Just (intercalate "\n\n" errorList)
            | otherwise = Nothing

loadSystem :: (String, [GenEntryDefinition]) -> Either String SystemData
loadSystem (name, entries) = case getError of
    Just e -> Left ("Error with system " ++ name ++ ": " ++ e)
    Nothing -> Right SystemData { systemName = name, families = rights getFamilies }
    where
        familyLists = [(name, vars) | Family name vars <- entries]
        getFamilies = map (loadFamily) familyLists
        errorList = lefts getFamilies
        getError
            | not $ null errorList = Just (intercalate "\n\n" errorList)
            | otherwise = Nothing

loadFamily :: (String, [FamilyComponentEntry]) -> Either String FamilyData
loadFamily (name, comps) = case getError of
    Just e -> Left e
    Nothing -> Right FamilyData { familyName = name, familyComponents = rights compTypes }
    where
        compTypes = map (loadVariableType) comps
        errorList = lefts compTypes
        getError
            | not $ null errorList = Just (intercalate "\n\n" errorList)
            | otherwise = Nothing

loadVariable :: VariableDeclaration -> Either String VariableData
loadVariable (rawName, varType) = case loadVariableType varType of 
    Left e -> Left ("Invalid type on variable " ++ rawName ++ ": " ++ e)
    Right t -> case getError of
        Just e -> Left ("Invalid name on variable " ++ rawName ++ ": " ++ e)
        Nothing -> Right VariableData { variableName = name, variableType = t }
        where
            name = tryHead (words rawName)
            getError
                | name == "" = Just "Empty variable name"
                | otherwise = Nothing

loadVariableType :: String -> Either String VariableTypeData
loadVariableType rawName = case getError of
    Just e -> Left ("Invalid type on variable " ++ rawName ++ ": " ++ e)
    Nothing -> Right VariableTypeData { typeName = name, Halley.SemanticAnalysis.const = isConst }
    where
        decorList = ["const"]
        name = tryHead undecor
        undecor = (words rawName) \\ decorList
        decors = (words rawName) `intersect` decorList
        isConst = "const" `elem` decors 
        getError
            | name == "" = Just "Empty type name"
            | length undecor /= 1 = Just $ "Variable type name seems to contain spaces: " ++ (intercalate " " undecor)
            | otherwise = Nothing

validate :: CodeGenData -> Maybe String
validate d
    | not $ null systemErrors = Just (intercalate "\n\n" systemErrors)
    | otherwise = Nothing
    where
        cs = components d
        sys = systems d
        systemErrors = catMaybes $ map (validateSystem cs) sys

validateSystem :: [ComponentData] -> SystemData -> Maybe String
validateSystem cs system
    | not $ null missingComponents = Just $ "Missing component in system " ++ (systemName system) ++ ": " ++ (intercalate " " missingComponents)
    | otherwise = Nothing
    where
        componentNames = map (componentName) cs
        missingComponents = map (typeName) (nub $ concat $ map (familyComponents) (families system)) \\ componentNames
        
tryHead [] = []
tryHead (x:_) = x
