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
, CodeGenData
, ComponentData
, SystemData
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
                                   , members :: [VariableData]
                                   , functions :: [FunctionData]
                                   } deriving (Show)

data SystemData = SystemData { systemName :: String
                             , family :: [VariableTypeData]
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
                                         } deriving (Show)
---------------------


semanticAnalysis :: [GenDefinition] -> Either String CodeGenData
semanticAnalysis gens = case doValidate loadedData of
    Just error -> Left error
    Nothing -> Right loadedData
    where
        cs = map (loadComponent) gens
        sys = map (loadSystem) gens
        loadedData = CodeGenData { components = clean cs, systems = clean sys }
        doValidate x = case lefts cs ++ lefts sys of
            [] -> validate x
            e:_ -> Just e

loadComponent :: GenDefinition -> Either String (Maybe ComponentData)
loadComponent (ComponentDefinition name entries) = case getError of
    Just e -> Left ("Error with component " ++ name ++ ": " ++ e)
    Nothing -> Right $ Just ComponentData { componentName = name, members = getMembers, functions = getFunctions }
    where
        getMembers = []
        getFunctions = []
        getError
            | False = Just "test"
            | otherwise = Nothing
loadComponent _ = Right Nothing

loadSystem :: GenDefinition -> Either String (Maybe SystemData)
loadSystem (SystemDefinition name entries) = case getError of
    Just e -> Left ("Error with system " ++ name ++ ": " ++ e)
    Nothing -> Right $ Just SystemData { systemName = name, family = getFamily }
    where
        getFamily = []
        getError = Nothing
loadSystem _ = Right Nothing

clean = catMaybes . rights

validate :: CodeGenData -> Maybe String
validate d = Nothing -- TODO

