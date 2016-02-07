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
module Halley.CodeGenCpp
( generateCodeCpp
) where

import Halley.CodeGen
import Halley.AST
import Halley.SemanticAnalysis
import Data.List
import Data.Char

---------------------


generateCodeCpp :: CodeGenData -> [GeneratedSource]
generateCodeCpp gen = comps ++ sys
    where
        comps = concat (map (genComponent) (components gen))
        sys = concat (map (genSystem) (systems gen))


---------------------


genVariable :: VariableData -> String
genVariable var = genType (variableType var) ++ " " ++ (variableName var)

genType :: VariableTypeData -> String
genType t = (if Halley.SemanticAnalysis.const t then "const " else "") ++ typeName t

genFunctionDeclaration :: FunctionData -> String
genFunctionDeclaration f = concat [genType (returnType f)
                                  ," "
                                  ,(functionName f)
                                  ," ("
                                  ,");"]

genMember :: VariableData -> String
genMember var = concat ["    "
                       ,genVariable var
                       ,";"]


---------------------


genComponent :: ComponentData -> [GeneratedSource]
genComponent compData = [ GeneratedSource { filename = basePath ++ ".h", code = header } ]
    where
        name = componentName compData
        basePath = "cpp/components/" ++ (makeUnderscores name)
        header = intercalate "\n" headerCode
        headerCode = ["#include \"component.h\""
                     ,"class " ++ name ++ " : public Component {"
                     ,"public:"
                     ,"    constexpr static int componentIndex = " ++ (show $ componentIndex compData) ++ ";"
                     ,""]
                     ++ memberList ++
                     [""]
                     ++ functionList ++
                     ["};"]
        memberList = map (genMember) (members compData)
        functionList = map (genFunctionDeclaration) (functions compData)

genSystem :: SystemData -> [GeneratedSource]
genSystem sysData = [GeneratedSource{filename = basePath ++ ".h", code = header}]
    where
        baseSystemClass = "System"
        name = systemName sysData
        basePath = "cpp/systems/" ++ (makeUnderscores name)
        header = intercalate "\n" headerCode
        headerCode = ["#include \"system.h\""
                     ,"class " ++ name ++ " : public " ++ baseSystemClass ++ " {"
                     ,"public:"
                     ,"    // TODO: this requires " ++ (show $ length $ families sysData) ++ " families."
                     ,"protected:"
                     ,"    void doStep() override;"
                     ,"};"]


---------------------


makeUnderscores :: String -> String
makeUnderscores [] = []
makeUnderscores (c:cs) = (toLower c) : (concat $ map (processChar) cs)
    where
        processChar c' = if isAsciiUpper c' then '_' : [toLower c'] else [c']